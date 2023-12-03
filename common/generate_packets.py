#!/usr/bin/env python3

#                      Copyright (c) 1996-2021 Freeciv21 and Freeciv
#     ,_,               contributors. This file is part of Freeciv21.
#    (0_0)_----------_           Freeciv21 is free software: you can
#   (_____)           |~'  redistribute it and/or modify it under the
#   `-"-"-'           /      terms of the GNU  General Public License
#     `|__|~-----~|__|   as published by the Free Software Foundation,
#                                   either version 3 of the  License,
#                  or (at your option) any later version. You should
#     have received  a copy of the GNU  General Public License along
#          with Freeciv21. If not, see https://www.gnu.org/licenses/.

import argparse
import io
import re

# The following parameters change the amount of output.

# generate_stats will generate a large amount of statistics how many
# info packets got discarded and how often a field is transmitted. You
# have to call delta_stats_report to get these.
generate_stats = 0

# generate_logs will generate log calls to debug the delta code.
generate_logs = 1
use_log_macro = "log_packet_detailed"
generate_variant_logs = 1

# The following parameters CHANGE the protocol. You have been warned.
fold_bool_into_header = 1

################# END OF PARAMETERS ####################


def prefix(prefix, string):
    lines = string.split("\n")
    lines = map(lambda x, prefix=prefix: prefix + x, lines)
    return "\n".join(lines)


def get_choices(allchoices):
    def helper(helper, allchoices, index, so_far):
        if index >= len(allchoices):
            return [so_far]
        t0 = so_far[:]
        t1 = so_far[:]
        t1.append(list(allchoices)[index])
        return helper(helper, allchoices, index + 1, t1) + helper(
            helper, allchoices, index + 1, t0
        )

    result = helper(helper, allchoices, 0, [])
    assert len(result) == 2 ** len(allchoices)
    return result


# A simple container for a type alias


class Type:
    def __init__(self, alias, dest):
        self.alias = alias
        self.dest = dest


# Parses a line of the form "COORD x, y; key" and returns a list of
# Field objects. types is a list of Type objects which are used to
# dereference type names.


def parse_fields(string, types):
    mo = re.search(r"^\s*(\S+(?:\(.*\))?)\s+([^;()]*)\s*;\s*(.*)\s*$", string)
    assert mo, string
    arr = []
    for i in mo.groups():
        if i:
            arr.append(i.strip())
        else:
            arr.append("")
    kind, fields_, flags = arr
    # print arr

    # analyze type
    while 1:
        found = 0
        for i in types:
            if i.alias == kind:
                kind = i.dest
                found = 1
                break
        if not found:
            break

    typeinfo = {}
    mo = re.search("^(.*)\((.*)\)$", kind)
    assert mo, repr(kind)
    typeinfo["dataio_type"], typeinfo["struct_type"] = mo.groups()

    if typeinfo["struct_type"] == "float":
        mo = re.search("^(\D+)(\d+)$", typeinfo["dataio_type"])
        assert mo
        typeinfo["dataio_type"] = mo.group(1)
        typeinfo["float_factor"] = int(mo.group(2))

    # analyze fields
    fields = []
    for i in fields_.split(","):
        i = i.strip()
        t = {}

        def f(x):
            arr = x.split(":")
            if len(arr) == 1:
                return [x, x, x]
            else:
                assert len(arr) == 2
                arr.append("old->" + arr[1])
                arr[1] = "real_packet->" + arr[1]
                return arr

        mo = re.search(r"^(.*)\[(.*)\]\[(.*)\]$", i)
        if mo:
            t["name"] = mo.group(1)
            t["is_array"] = 2
            t["array_size1_d"], t["array_size1_u"], t["array_size1_o"] = f(mo.group(2))
            t["array_size2_d"], t["array_size2_u"], t["array_size2_o"] = f(mo.group(3))
        else:
            mo = re.search(r"^(.*)\[(.*)\]$", i)
            if mo:
                t["name"] = mo.group(1)
                t["is_array"] = 1
                t["array_size_d"], t["array_size_u"], t["array_size_o"] = f(mo.group(2))
            else:
                t["name"] = i
                t["is_array"] = 0
        fields.append(t)

    # analyze flags
    flaginfo = {}
    arr = list(item.strip() for item in flags.split(","))
    arr = list(filter(lambda x: len(x) > 0, arr))
    flaginfo["is_key"] = "key" in arr
    if flaginfo["is_key"]:
        arr.remove("key")
    flaginfo["diff"] = "diff" in arr
    if flaginfo["diff"]:
        arr.remove("diff")
    adds = []
    removes = []
    remaining = []
    for i in arr:
        mo = re.search("^add-cap\((.*)\)$", i)
        if mo:
            adds.append(mo.group(1))
            continue
        mo = re.search("^remove-cap\((.*)\)$", i)
        if mo:
            removes.append(mo.group(1))
            continue
        remaining.append(i)
    arr = remaining
    assert len(arr) == 0, repr(arr)
    assert len(adds) + len(removes) in [0, 1]

    if adds:
        flaginfo["add_cap"] = adds[0]
    else:
        flaginfo["add_cap"] = ""

    if removes:
        flaginfo["remove_cap"] = removes[0]
    else:
        flaginfo["remove_cap"] = ""

    # print typeinfo,flaginfo,fields
    result = []
    for f in fields:
        result.append(Field(f, typeinfo, flaginfo))
    return result


# Class for a field (part of a packet). It has a name, serveral types,
# flags and some other attributes.


class Field:
    def __init__(self, fieldinfo, typeinfo, flaginfo):
        for i in fieldinfo, typeinfo, flaginfo:
            self.__dict__.update(i)
        self.is_struct = re.search("^struct.*", self.struct_type)

    # Helper function for the dictionary variant of the % operator
    # ("%(name)s"%dict).
    def get_dict(self, params):
        result = self.__dict__.copy()
        result.update(params)
        return result

    def get_handle_type(self):
        if self.dataio_type == "string" or self.dataio_type == "estring":
            return "const char *"
        if self.dataio_type == "worklist":
            return "const %s *" % self.struct_type
        if self.is_array:
            return "const %s *" % self.struct_type
        return self.struct_type + " "

    # Returns code which is used in the declaration of the field in
    # the packet struct.
    def get_declar(self):
        if self.is_array == 2:
            return (
                "%(struct_type)s %(name)s[%(array_size1_d)s][%(array_size2_d)s]"
                % self.__dict__
            )
        if self.is_array:
            return "%(struct_type)s %(name)s[%(array_size_d)s]" % self.__dict__
        else:
            return "%(struct_type)s %(name)s" % self.__dict__

    # Returns code which copies the arguments of the direct send
    # functions in the packet struct.
    def get_fill(self):
        if self.dataio_type == "worklist":
            return "  worklist_copy(&real_packet->%(name)s, %(name)s);" % self.__dict__
        if self.is_array == 0:
            return "  real_packet->%(name)s = %(name)s;" % self.__dict__
        if self.dataio_type == "string" or self.dataio_type == "estring":
            return "  sz_strlcpy(real_packet->%(name)s, %(name)s);" % self.__dict__
        if self.is_array == 1:
            tmp = "real_packet->%(name)s[i] = %(name)s[i]" % self.__dict__
            return """  {
    int i;

    for (i = 0; i < %(array_size_u) s; i++) {
      %(tmp)s;
    }
  }""" % self.get_dict(
                vars()
            )

        return repr(self.__dict__)

    # Returns code which sets "differ" by comparing the field
    # instances of "old" and "readl_packet".
    def get_cmp(self):
        if self.dataio_type == "memory":
            return (
                "  differ = (memcmp(old->%(name)s, real_packet->%(name)s, %(array_size_d)s) != 0);"
                % self.__dict__
            )
        if self.dataio_type == "bitvector":
            return (
                "  differ = !BV_ARE_EQUAL(old->%(name)s, real_packet->%(name)s);"
                % self.__dict__
            )
        if self.dataio_type in ["string", "estring"] and self.is_array == 1:
            return (
                "  differ = (strcmp(old->%(name)s, real_packet->%(name)s) != 0);"
                % self.__dict__
            )
        if self.dataio_type == "cm_parameter":
            return (
                "  differ = (&old->%(name)s != &real_packet->%(name)s);" % self.__dict__
            )
        if self.is_struct and self.is_array == 0:
            return (
                "  differ = !are_%(dataio_type)ss_equal(&old->%(name)s, &real_packet->%(name)s);"
                % self.__dict__
            )
        if not self.is_array:
            return (
                "  differ = (old->%(name)s != real_packet->%(name)s);" % self.__dict__
            )

        sizes = None, None
        if self.dataio_type == "string" or self.dataio_type == "estring":
            c = (
                "strcmp(old->%(name)s[i], real_packet->%(name)s[i]) != 0"
                % self.__dict__
            )
            sizes = self.array_size1_o, self.array_size1_u
        elif self.is_struct:
            c = (
                "!are_%(dataio_type)ss_equal(&old->%(name)s[i], &real_packet->%(name)s[i])"
                % self.__dict__
            )
            sizes = self.array_size_o, self.array_size_u
        else:
            c = "old->%(name)s[i] != real_packet->%(name)s[i]" % self.__dict__
            sizes = self.array_size_o, self.array_size_u

        differ = f"({sizes[0]} != {sizes[1]})"
        if sizes[0] == sizes[1]:
            differ = "false"
        return f"""
    {{
      differ = {differ};
      if (!differ) {{
        int i;

        for (i = 0; i < {sizes[1]}; i++) {{
          if ({c}) {{
            differ = true;
            break;
          }}
        }}
      }}
    }}"""

    # Returns a code fragment which updates the bit of the this field
    # in the "fields" bitvector. The bit is either a "content-differs"
    # bit or (for bools which gets folded in the header) the actual
    # value of the bool.
    def get_cmp_wrapper(self, i):
        cmp = self.get_cmp()
        if fold_bool_into_header and self.struct_type == "bool" and not self.is_array:
            b = "packet->%(name)s" % self.get_dict(vars())
            return """%s
  if (differ) {
    different++;
  }
  if (%s) {
    BV_SET(fields, %d);
  }

""" % (
                cmp,
                b,
                i,
            )

        return """%s
  if (differ) {
    different++;
    BV_SET(fields, %d);
  }

""" % (
            cmp,
            i,
        )

    # Returns a code fragment which will put this field if the
    # content has changed. Does nothing for bools-in-header.
    def get_put_wrapper(self, packet, i, deltafragment):
        if fold_bool_into_header and self.struct_type == "bool" and not self.is_array:
            return "  /* field %(i)d is folded into the header */\n" % vars()
        put = self.get_put(deltafragment)
        packet_name = packet.name
        log_macro = packet.log_macro
        if packet.gen_log:
            f = (
                "    %(log_macro)s(\"  field '%(name)s' has changed\");\n"
                % self.get_dict(vars())
            )
        else:
            f = ""
        if packet.gen_stats:
            s = "    stats_%(packet_name)s_counters[%(i)d]++;\n" % self.get_dict(vars())
        else:
            s = ""
        return """  if (BV_ISSET(fields, %(i)d)) {
%(f)s%(s)s  %(put)s
  }
""" % self.get_dict(
            vars()
        )

    # Returns code which put this field.
    def get_put(self, deltafragment):
        if self.dataio_type == "bitvector":
            return "DIO_BV_PUT(&dout, &field_addr, packet->%(name)s);" % self.__dict__

        if self.struct_type == "float" and not self.is_array:
            return (
                "  DIO_PUT(%(dataio_type)s, &dout, &field_addr, real_packet->%(name)s, %(float_factor)d);"
                % self.__dict__
            )

        if self.dataio_type in ["worklist", "cm_parameter"]:
            return (
                "  DIO_PUT(%(dataio_type)s, &dout, &field_addr, &real_packet->%(name)s);"
                % self.__dict__
            )

        if self.dataio_type in ["memory"]:
            return (
                "  DIO_PUT(%(dataio_type)s, &dout, &field_addr, &real_packet->%(name)s, %(array_size_u)s);"
                % self.__dict__
            )

        arr_types = ["string", "estring", "city_map"]
        if (self.dataio_type in arr_types and self.is_array == 1) or (
            self.dataio_type not in arr_types and self.is_array == 0
        ):
            return (
                "  DIO_PUT(%(dataio_type)s, &dout, &field_addr, real_packet->%(name)s);"
                % self.__dict__
            )
        if self.is_struct:
            if self.is_array == 2:
                c = (
                    "DIO_PUT(%(dataio_type)s, &dout, &field_addr, &real_packet->%(name)s[i][j]);"
                    % self.__dict__
                )
            else:
                c = (
                    "DIO_PUT(%(dataio_type)s, &dout, &field_addr, &real_packet->%(name)s[i]);"
                    % self.__dict__
                )
        elif self.dataio_type == "string" or self.dataio_type == "estring":
            c = (
                "DIO_PUT(%(dataio_type)s, &dout, &field_addr, real_packet->%(name)s[i]);"
                % self.__dict__
            )
            array_size_u = self.array_size1_u

        elif self.struct_type == "float":
            if self.is_array == 2:
                c = (
                    "  DIO_PUT(%(dataio_type)s, &dout, &field_addr, real_packet->%(name)s[i][j], %(float_factor)d);"
                    % self.__dict__
                )
            else:
                c = (
                    "  DIO_PUT(%(dataio_type)s, &dout, &field_addr, real_packet->%(name)s[i], %(float_factor)d);"
                    % self.__dict__
                )
        else:
            if self.is_array == 2:
                c = (
                    "DIO_PUT(%(dataio_type)s, &dout, &field_addr, real_packet->%(name)s[i][j]);"
                    % self.__dict__
                )
            else:
                c = (
                    "DIO_PUT(%(dataio_type)s, &dout, &field_addr, real_packet->%(name)s[i]);"
                    % self.__dict__
                )

        if deltafragment and self.diff and self.is_array == 1:
            return """
    {
      int i;

      fc_assert(%(array_size_u)s < 255);

      for (i = 0; i < %(array_size_u)s; i++) {
        if (old->%(name)s[i] != real_packet->%(name)s[i]) {
          DIO_PUT(uint8, &dout, &field_addr, i);

          %(c)s
        }
      }
      DIO_PUT(uint8, &dout, &field_addr, 255);

    }""" % self.get_dict(
                vars()
            )
        if (
            self.is_array == 2
            and self.dataio_type != "string"
            and self.dataio_type != "estring"
        ):
            return """
    {
      int i, j;

      for (i = 0; i < %(array_size1_u)s; i++) {
        for (j = 0; j < %(array_size2_u)s; j++) {
          %(c)s
        }
      }
    }""" % self.get_dict(
                vars()
            )

        return """
    {
      int i;

      for (i = 0; i < %(array_size_u)s; i++) {
        %(c)s
      }
    }""" % self.get_dict(
            vars()
        )

    # Returns a code fragment which will get the field if the
    # "fields" bitvector says so.
    def get_get_wrapper(self, packet, i, deltafragment):
        get = self.get_get(deltafragment)
        if fold_bool_into_header and self.struct_type == "bool" and not self.is_array:
            return (
                "  real_packet->%(name)s = BV_ISSET(fields, %(i)d);\n"
                % self.get_dict(vars())
            )
        get = prefix("    ", get)
        log_macro = packet.log_macro
        if packet.gen_log:
            f = "    %(log_macro)s(\"  got field '%(name)s'\");\n" % self.get_dict(
                vars()
            )
        else:
            f = ""
        return """  if (BV_ISSET(fields, %(i)d)) {
%(f)s%(get)s
  }
""" % self.get_dict(
            vars()
        )

    # Returns code which get this field.
    def get_get(self, deltafragment):
        if self.struct_type == "float" and not self.is_array:
            return (
                """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &real_packet->%(name)s, %(float_factor)d)) {
  RECEIVE_PACKET_FIELD_ERROR(%(name)s);
}"""
                % self.__dict__
            )
        if self.dataio_type == "bitvector":
            return (
                """if (!DIO_BV_GET(&din, &field_addr, real_packet->%(name)s)) {
  RECEIVE_PACKET_FIELD_ERROR(%(name)s);
}"""
                % self.__dict__
            )
        if self.dataio_type in ["string", "estring", "city_map"] and self.is_array != 2:
            return (
                """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, real_packet->%(name)s, sizeof(real_packet->%(name)s))) {
  RECEIVE_PACKET_FIELD_ERROR(%(name)s);
}"""
                % self.__dict__
            )
        if self.is_struct and self.is_array == 0:
            return (
                """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &real_packet->%(name)s)) {
  RECEIVE_PACKET_FIELD_ERROR(%(name)s);
}"""
                % self.__dict__
            )
        if not self.is_array:
            if self.struct_type in ["int", "bool"]:
                return (
                    """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &real_packet->%(name)s)) {
  RECEIVE_PACKET_FIELD_ERROR(%(name)s);
}"""
                    % self.__dict__
                )

            return (
                """{
  int readin;

  if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &readin)) {
    RECEIVE_PACKET_FIELD_ERROR(%(name)s);
  }
  real_packet->%(name)s = static_cast<decltype(real_packet->%(name)s)>(readin);
}"""
                % self.__dict__
            )

        if self.is_struct:
            if self.is_array == 2:
                c = (
                    """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &real_packet->%(name)s[i][j])) {
      RECEIVE_PACKET_FIELD_ERROR(%(name)s);
    }"""
                    % self.__dict__
                )
            else:
                c = (
                    """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &real_packet->%(name)s[i])) {
      RECEIVE_PACKET_FIELD_ERROR(%(name)s);
    }"""
                    % self.__dict__
                )
        elif self.dataio_type == "string" or self.dataio_type == "estring":
            c = (
                """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, real_packet->%(name)s[i], sizeof(real_packet->%(name)s[i]))) {
      RECEIVE_PACKET_FIELD_ERROR(%(name)s);
    }"""
                % self.__dict__
            )
        elif self.struct_type == "float":
            if self.is_array == 2:
                c = (
                    """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &real_packet->%(name)s[i][j], %(float_factor)d)) {
      RECEIVE_PACKET_FIELD_ERROR(%(name)s);
    }"""
                    % self.__dict__
                )
            else:
                c = (
                    """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &real_packet->%(name)s[i], %(float_factor)d)) {
      RECEIVE_PACKET_FIELD_ERROR(%(name)s);
    }"""
                    % self.__dict__
                )
        elif self.is_array == 2:
            if self.struct_type in ["int", "bool"]:
                c = (
                    """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &real_packet->%(name)s[i][j])) {
      RECEIVE_PACKET_FIELD_ERROR(%(name)s);
    }"""
                    % self.__dict__
                )
            else:
                c = (
                    """{
      int readin;

      if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &readin)) {
        RECEIVE_PACKET_FIELD_ERROR(%(name)s);
      }
      real_packet->%(name)s[i][j] = readin;
    }"""
                    % self.__dict__
                )
        elif self.struct_type in ["int", "bool"]:
            c = (
                """if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &real_packet->%(name)s[i])) {
      RECEIVE_PACKET_FIELD_ERROR(%(name)s);
    }"""
                % self.__dict__
            )
        else:
            c = (
                """{
      int readin;

      if (!DIO_GET(%(dataio_type)s, &din, &field_addr, &readin)) {
        RECEIVE_PACKET_FIELD_ERROR(%(name)s);
      }
      real_packet->%(name)s[i] = readin;
    }"""
                % self.__dict__
            )

        if self.is_array == 2:
            array_size_u = self.array_size1_u
            array_size_d = self.array_size1_d
        else:
            array_size_u = self.array_size_u
            array_size_d = self.array_size_d

        if not self.diff or self.dataio_type == "memory":
            if array_size_u != array_size_d:
                extra = """
  if (%(array_size_u)s > %(array_size_d)s) {
    RECEIVE_PACKET_FIELD_ERROR(%(name)s, ": truncation array");
  }""" % self.get_dict(
                    vars()
                )
            else:
                extra = ""
            if self.dataio_type == "memory":
                return """%(extra)s
  if (!DIO_GET(%(dataio_type)s, &din, &field_addr, real_packet->%(name)s, %(array_size_u)s)) {
    RECEIVE_PACKET_FIELD_ERROR(%(name)s);
  }""" % self.get_dict(
                    vars()
                )
            if (
                self.is_array == 2
                and self.dataio_type != "string"
                and self.dataio_type != "estring"
            ):
                return """
{
  int i, j;

%(extra)s
  for (i = 0; i < %(array_size1_u)s; i++) {
    for (j = 0; j < %(array_size2_u)s; j++) {
      %(c)s
    }
  }
}""" % self.get_dict(
                    vars()
                )
            else:
                return """
{
  int i;

%(extra)s
  for (i = 0; i < %(array_size_u)s; i++) {
    %(c)s
  }
}""" % self.get_dict(
                    vars()
                )
        elif deltafragment and self.diff and self.is_array == 1:
            return """
{
int count;

for (count = 0;; count++) {
  int i;

  if (!DIO_GET(uint8, &din, &field_addr, &i)) {
    RECEIVE_PACKET_FIELD_ERROR(%(name)s);
  }
  if (i == 255) {
    break;
  }
  if (i >= %(array_size_u)s) {
    RECEIVE_PACKET_FIELD_ERROR(%(name)s,
                               \": unexpected value %%%%d \"
                               \"(> %(array_size_u)s) in array diff\",
                               i);
  } else {
    %(c)s
  }
}
}""" % self.get_dict(
                vars()
            )
        else:
            return """
{
  int i;

  for (i = 0; i < %(array_size_u)s; i++) {
    %(c)s
  }
}""" % self.get_dict(
                vars()
            )


# Class which represents a capability variant.
class Variant:
    def __init__(self, poscaps, negcaps, name, fields, packet, no):
        self.log_macro = use_log_macro
        self.gen_stats = generate_stats
        self.gen_log = generate_logs
        self.name = name
        self.packet_name = packet.name
        self.fields = fields
        self.no = no

        self.no_packet = packet.no_packet
        self.want_post_recv = packet.want_post_recv
        self.want_pre_send = packet.want_pre_send
        self.want_post_send = packet.want_post_send
        self.type = packet.type
        self.delta = packet.delta
        self.is_info = packet.is_info
        self.cancel = packet.cancel
        self.want_force = packet.want_force

        self.poscaps = poscaps
        self.negcaps = negcaps
        if self.poscaps or self.negcaps:

            def f(cap):
                return 'has_capability("%s", capability)' % (cap)

            t = list(map(lambda x, f=f: f(x), self.poscaps)) + list(
                map(lambda x, f=f: "!" + f(x), self.negcaps)
            )
            self.condition = " && ".join(t)
        else:
            self.condition = "true"
        self.key_fields = list(filter(lambda x: x.is_key, self.fields))
        self.other_fields = list(filter(lambda x: not x.is_key, self.fields))
        self.bits = len(self.other_fields)
        self.keys_format = ", ".join(["%d"] * len(self.key_fields))
        self.keys_arg = ", ".join(
            map(lambda x: "real_packet->" + x.name, self.key_fields)
        )
        if self.keys_arg:
            self.keys_arg = ",\n    " + self.keys_arg

        if len(self.fields) == 0:
            self.delta = 0
            self.no_packet = 1

        if len(self.fields) > 5 or self.name.split("_")[1] == "ruleset":
            self.handle_via_packet = 1

        self.extra_send_args = ""
        self.extra_send_args2 = ""
        self.extra_send_args3 = ", ".join(
            map(lambda x: "%s%s" % (x.get_handle_type(), x.name), self.fields)
        )
        if self.extra_send_args3:
            self.extra_send_args3 = ", " + self.extra_send_args3

        if not self.no_packet:
            self.extra_send_args = (
                ", const struct %(packet_name)s *packet" % self.__dict__
                + self.extra_send_args
            )
            self.extra_send_args2 = ", packet" + self.extra_send_args2

        if self.want_force:
            self.extra_send_args = self.extra_send_args + ", bool force_to_send"
            self.extra_send_args2 = self.extra_send_args2 + ", force_to_send"
            self.extra_send_args3 = self.extra_send_args3 + ", bool force_to_send"

        self.receive_prototype = (
            "static struct %(packet_name)s *receive_%(name)s(struct connection *pc)"
            % self.__dict__
        )
        self.send_prototype = (
            "static int send_%(name)s(struct connection *pc%(extra_send_args)s)"
            % self.__dict__
        )

        if self.no_packet:
            self.send_handler = (
                "phandlers->send[%(type)s].no_packet = (int(*)(struct connection *)) send_%(name)s;"
                % self.__dict__
            )
        elif self.want_force:
            self.send_handler = (
                "phandlers->send[%(type)s].force_to_send = (int(*)(struct connection *, const void *, bool)) send_%(name)s;"
                % self.__dict__
            )
        else:
            self.send_handler = (
                "phandlers->send[%(type)s].packet = (int(*)(struct connection *, const void *)) send_%(name)s;"
                % self.__dict__
            )
        self.receive_handler = (
            "phandlers->receive[%(type)s] = (void *(*)(struct connection *)) receive_%(name)s;"
            % self.__dict__
        )

    # See Field.get_dict
    def get_dict(self, params):
        result = self.__dict__.copy()
        result.update(params)
        return result

    # Returns a code fragment which contains the declarations of the
    # statistical counters of this packet.
    def get_stats(self):
        names = map(lambda x: '"' + x.name + '"', self.other_fields)
        names = ", ".join(names)

        return """static int stats_%(name)s_sent;
static int stats_%(name)s_discarded;
static int stats_%(name)s_counters[%(bits)d];
static char *stats_%(name)s_names[] = {%(names)s};

""" % self.get_dict(
            vars()
        )

    # Returns a code fragment which declares the packet specific
    # bitvector. Each bit in this bitvector represents one non-key
    # field.
    def get_bitvector(self):
        return "BV_DEFINE(%(name)s_fields, %(bits)d);\n" % self.__dict__

    # Returns a code fragment which is the packet specific part of
    # the delta_stats_report() function.
    def get_report_part(self):
        return (
            """
  if (stats_%(name)s_sent > 0
      && stats_%(name)s_discarded != stats_%(name)s_sent) {
    log_test(\"%(name)s %%d out of %%d got discarded\",
      stats_%(name)s_discarded, stats_%(name)s_sent);
    for (i = 0; i < %(bits)d; i++) {
      if (stats_%(name)s_counters[i] > 0) {
        log_test(\"  %%4d / %%4d: %%2d = %%s\",
          stats_%(name)s_counters[i],
          (stats_%(name)s_sent - stats_%(name)s_discarded),
          i, stats_%(name)s_names[i]);
      }
    }
  }
"""
            % self.__dict__
        )

    # Returns a code fragment which is the packet specific part of
    # the delta_stats_reset() function.
    def get_reset_part(self):
        return (
            """
  stats_%(name)s_sent = 0;
  stats_%(name)s_discarded = 0;
  memset(stats_%(name)s_counters, 0,
         sizeof(stats_%(name)s_counters));
"""
            % self.__dict__
        )

    # Returns a code fragment which is the implementation of the hash
    # function. The hash function is using all key fields.
    def get_hash(self):
        if len(self.key_fields) == 0:
            return "#define hash_%(name)s hash_const\n\n" % self.__dict__

        intro = (
            """static genhash_val_t hash_%(name)s(const void *vkey)
{
"""
            % self.__dict__
        )

        body = (
            """  const struct %(packet_name)s *key = (const struct %(packet_name)s *) vkey;

"""
            % self.__dict__
        )

        keys = list(map(lambda x: "key->" + x.name, self.key_fields))
        if len(keys) == 1:
            a = keys[0]
        elif len(keys) == 2:
            a = "(%s << 8) ^ %s" % (keys[0], keys[1])
        else:
            assert 0
        body = body + ("  return %s;\n" % a)
        extro = "}\n\n"
        return intro + body + extro

    # Returns a code fragment which is the implementation of the cmp
    # function. The cmp function is using all key fields. The cmp
    # function is used for the hash table.
    def get_cmp(self):
        if len(self.key_fields) == 0:
            return "#define cmp_%(name)s cmp_const\n\n" % self.__dict__

        intro = (
            """static bool cmp_%(name)s(const void *vkey1, const void *vkey2)
{
"""
            % self.__dict__
        )
        body = ""
        body = (
            body
            + """  const struct %(packet_name)s *key1 = (const struct %(packet_name)s *) vkey1;
  const struct %(packet_name)s *key2 = (const struct %(packet_name)s *) vkey2;

"""
            % self.__dict__
        )
        for field in self.key_fields:
            body = (
                body
                + """  return key1->%s == key2->%s;
"""
                % (field.name, field.name)
            )
        extro = "}\n"
        return intro + body + extro

    # Returns a code fragment which is the implementation of the send
    # function. This is one of the two real functions. So it is rather
    # complex to create.
    def get_send(self):
        temp = """%(send_prototype)s
{
<real_packet1><delta_header>  SEND_PACKET_START(%(type)s);
<faddr><log><report><pre1><body><pre2><post>  SEND_PACKET_END(%(type)s);
}

"""
        if self.gen_stats:
            report = """
  stats_total_sent++;
  stats_%(name)s_sent++;
"""
        else:
            report = ""
        if self.gen_log:
            log = '\n  %(log_macro)s("%(name)s: sending info about (%(keys_format)s)"%(keys_arg)s);\n'
        else:
            log = ""
        if self.want_pre_send:
            pre1 = """
  {
    auto tmp = new %(packet_name)s;

    *tmp = *packet;
    pre_send_%(packet_name)s(pc, tmp);
    real_packet = tmp;
  }
"""
            pre2 = """
  if (real_packet != packet) {
    delete (decltype(real_packet)) real_packet;
  }
"""
        else:
            pre1 = ""
            pre2 = ""

        if not self.no_packet:
            real_packet1 = "  const struct %(packet_name)s *real_packet = packet;\n"
        else:
            real_packet1 = ""

        if not self.no_packet:
            if self.delta:
                if self.want_force:
                    diff = "force_to_send"
                else:
                    diff = "0"
                delta_header = """
  %(name)s_fields fields;
  struct %(packet_name)s *old;
  bool differ;
  struct genhash **hash = pc->phs.sent + %(type)s;
  int different = %(diff)s;
"""
                body = self.get_delta_send_body()
            else:
                delta_header = ""
                body = ""
                for field in self.fields:
                    body = body + field.get_put(0) + "\n"
            body = body + "\n"
        else:
            body = ""
            delta_header = ""

        if self.want_post_send:
            if self.no_packet:
                post = "  post_send_%(packet_name)s(pc, NULL);\n"
            else:
                post = "  post_send_%(packet_name)s(pc, real_packet);\n"
        else:
            post = ""

        faddr = ""

        for i in range(2):
            for k, v in vars().items():
                if isinstance(v, str):
                    temp = temp.replace("<%s>" % k, v)
        return temp % self.get_dict(vars())

    # '''

    # Helper for get_send()
    def get_delta_send_body(self):
        intro = """
  if (NULL == *hash) {
    *hash = genhash_new_full(hash_%(name)s, cmp_%(name)s,
                             NULL, NULL, NULL, free);
  }
  BV_CLR_ALL(fields);

  if (!genhash_lookup(*hash, real_packet, (void **) &old)) {
    old = new %(packet_name)s;
    *old = *real_packet;
    genhash_insert(*hash, old, old);
    memset(old, 0, sizeof(*old));
    different = 1;      /* Force to send. */
  }
"""
        body = ""
        for i in range(len(self.other_fields)):
            field = self.other_fields[i]
            body = body + field.get_cmp_wrapper(i)
        if self.gen_log:
            fl = '    %(log_macro)s("  no change -> discard");\n'
        else:
            fl = ""
        if self.gen_stats:
            s = "    stats_%(name)s_discarded++;\n"
        else:
            s = ""

        if self.is_info != "no":
            body = (
                body
                + """
  if (different == 0) {
%(fl)s%(s)s<pre2>    return 0;
  }
"""
                % self.get_dict(vars())
            )

        body = (
            body
            + """
  DIO_BV_PUT(&dout, &field_addr, fields);
"""
        )

        for field in self.key_fields:
            body = body + field.get_put(1) + "\n"
        body = body + "\n"

        for i in range(len(self.other_fields)):
            field = self.other_fields[i]
            body = body + field.get_put_wrapper(self, i, 1)
        body = (
            body
            + """
  *old = *real_packet;
"""
        )

        # Cancel some is-info packets.
        for i in self.cancel:
            body = (
                body
                + """
  hash = pc->phs.sent + %s;
  if (NULL != *hash) {
    genhash_remove(*hash, real_packet);
  }
"""
                % i
            )

        return intro + body

    # Returns a code fragment which is the implementation of the receive
    # function. This is one of the two real functions. So it is rather
    # complex to create.
    def get_receive(self):
        temp = """%(receive_prototype)s
{
<delta_header>  RECEIVE_PACKET_START(%(packet_name)s, real_packet);
<faddr><delta_body1><body1><log><body2><post>  RECEIVE_PACKET_END(real_packet);
}

"""
        if self.delta:
            delta_header = """
  %(name)s_fields fields;
  struct %(packet_name)s *old;
  struct genhash **hash = pc->phs.received + %(type)s;
"""
            delta_body1 = """
  DIO_BV_GET(&din, &field_addr, fields);
  """
            body1 = ""
            for field in self.key_fields:
                body1 = body1 + prefix("  ", field.get_get(1)) + "\n"
            body2 = self.get_delta_receive_body()
        else:
            delta_header = ""
            delta_body1 = ""
            body1 = ""
            for field in self.fields:
                body1 = body1 + prefix("  ", field.get_get(0)) + "\n"
            if not body1:
                body1 = "  real_packet->__dummy = 0xff;"
            body2 = ""
        body1 = body1 + "\n"

        if self.gen_log:
            log = '  %(log_macro)s("%(name)s: got info about (%(keys_format)s)"%(keys_arg)s);\n'
        else:
            log = ""

        if self.want_post_recv:
            post = "  post_receive_%(packet_name)s(pc, real_packet);\n"
        else:
            post = ""

        faddr = ""

        for i in range(2):
            for k, v in vars().items():
                if isinstance(v, str):
                    temp = temp.replace("<%s>" % k, v)
        return temp % self.get_dict(vars())

    # Helper for get_receive()
    def get_delta_receive_body(self):
        key1 = map(
            lambda x: "    %s %s = real_packet->%s;" % (x.struct_type, x.name, x.name),
            self.key_fields,
        )
        key2 = map(
            lambda x: "    real_packet->%s = %s;" % (x.name, x.name), self.key_fields
        )
        key1 = "\n".join(key1)
        key2 = "\n".join(key2)
        if key1:
            key1 = key1 + "\n\n"
        if key2:
            key2 = "\n\n" + key2
        if self.gen_log:
            fl = '    %(log_macro)s("  no old info");\n'
        else:
            fl = ""
        body = """
  if (NULL == *hash) {
    *hash = genhash_new_full(hash_%(name)s, cmp_%(name)s,
                             NULL, NULL, NULL, free);
  }

  if (genhash_lookup(*hash, real_packet, (void **) &old)) {
    *real_packet = *old;
  } else {
%(key1)s%(fl)s    memset(real_packet, 0, sizeof(*real_packet));%(key2)s
  }

""" % self.get_dict(
            vars()
        )
        for i in range(len(self.other_fields)):
            field = self.other_fields[i]
            body = body + field.get_get_wrapper(self, i, 1)

        extro = """
  if (NULL == old) {
    old = new %(packet_name)s;
    *old = *real_packet;
    genhash_insert(*hash, old, old);
  } else {
    *old = *real_packet;
  }
""" % self.get_dict(
            vars()
        )

        # Cancel some is-info packets.
        for i in self.cancel:
            extro = (
                extro
                + """
  hash = pc->phs.received + %s;
  if (NULL != *hash) {
    genhash_remove(*hash, real_packet);
  }
"""
                % i
            )

        return body + extro


# Class which represents a packet. A packet contains a list of fields.


class Packet:
    def __init__(self, string, types):
        self.types = types
        self.log_macro = use_log_macro
        self.gen_stats = generate_stats
        self.gen_log = generate_logs
        string = string.strip()
        lines = string.split("\n")

        mo = re.search("^\s*(\S+)\s*=\s*(\d+)\s*;\s*(.*?)\s*$", lines[0])
        assert mo, repr(lines[0])

        self.type = mo.group(1)
        self.name = self.type.lower()
        self.type_number = int(mo.group(2))
        assert 0 <= self.type_number <= 65535
        dummy = mo.group(3)

        del lines[0]

        arr = list(item.strip() for item in dummy.split(",") if item)

        self.dirs = []

        if "sc" in arr:
            self.dirs.append("sc")
            arr.remove("sc")
        if "cs" in arr:
            self.dirs.append("cs")
            arr.remove("cs")
        assert len(self.dirs) > 0, repr(self.name) + repr(self.dirs)

        # "no" means normal packet
        # "yes" means is-info packet
        # "game" means is-game-info packet
        self.is_info = "no"
        if "is-info" in arr:
            self.is_info = "yes"
            arr.remove("is-info")
        if "is-game-info" in arr:
            self.is_info = "game"
            arr.remove("is-game-info")

        self.want_pre_send = "pre-send" in arr
        if self.want_pre_send:
            arr.remove("pre-send")

        self.want_post_recv = "post-recv" in arr
        if self.want_post_recv:
            arr.remove("post-recv")

        self.want_post_send = "post-send" in arr
        if self.want_post_send:
            arr.remove("post-send")

        self.delta = "no-delta" not in arr
        if not self.delta:
            arr.remove("no-delta")

        self.no_packet = "no-packet" in arr
        if self.no_packet:
            arr.remove("no-packet")

        self.handle_via_packet = "handle-via-packet" in arr
        if self.handle_via_packet:
            arr.remove("handle-via-packet")

        self.handle_per_conn = "handle-per-conn" in arr
        if self.handle_per_conn:
            arr.remove("handle-per-conn")

        self.no_handle = "no-handle" in arr
        if self.no_handle:
            arr.remove("no-handle")

        self.dsend_given = "dsend" in arr
        if self.dsend_given:
            arr.remove("dsend")

        self.want_lsend = "lsend" in arr
        if self.want_lsend:
            arr.remove("lsend")

        self.want_force = "force" in arr
        if self.want_force:
            arr.remove("force")

        self.cancel = []
        removes = []
        remaining = []
        for i in arr:
            mo = re.search("^cancel\((.*)\)$", i)
            if mo:
                self.cancel.append(mo.group(1))
                continue
            remaining.append(i)
        arr = remaining

        assert len(arr) == 0, repr(arr)

        self.fields = []
        for i in lines:
            self.fields = self.fields + parse_fields(i, types)
        self.key_fields = list(filter(lambda x: x.is_key, self.fields))
        self.other_fields = list(filter(lambda x: not x.is_key, self.fields))
        self.bits = len(self.other_fields)
        self.keys_format = ", ".join(["%d"] * len(self.key_fields))
        self.keys_arg = ", ".join(
            map(lambda x: "real_packet->" + x.name, self.key_fields)
        )
        if self.keys_arg:
            self.keys_arg = ",\n    " + self.keys_arg

        self.want_dsend = self.dsend_given

        if len(self.fields) == 0:
            self.delta = 0
            self.no_packet = 1
            assert not self.want_dsend, "dsend for a packet without fields isn't useful"

        if len(self.fields) > 5 or self.name.split("_")[1] == "ruleset":
            self.handle_via_packet = 1

        self.extra_send_args = ""
        self.extra_send_args2 = ""
        self.extra_send_args3 = ", ".join(
            map(lambda x: "%s%s" % (x.get_handle_type(), x.name), self.fields)
        )
        if self.extra_send_args3:
            self.extra_send_args3 = ", " + self.extra_send_args3

        if not self.no_packet:
            self.extra_send_args = (
                ", const struct %(name)s *packet" % self.__dict__ + self.extra_send_args
            )
            self.extra_send_args2 = ", packet" + self.extra_send_args2

        if self.want_force:
            self.extra_send_args = self.extra_send_args + ", bool force_to_send"
            self.extra_send_args2 = self.extra_send_args2 + ", force_to_send"
            self.extra_send_args3 = self.extra_send_args3 + ", bool force_to_send"

        self.send_prototype = (
            "int send_%(name)s(struct connection *pc%(extra_send_args)s)"
            % self.__dict__
        )
        if self.want_lsend:
            self.lsend_prototype = (
                "void lsend_%(name)s(struct conn_list *dest%(extra_send_args)s)"
                % self.__dict__
            )
        if self.want_dsend:
            self.dsend_prototype = (
                "int dsend_%(name)s(struct connection *pc%(extra_send_args3)s)"
                % self.__dict__
            )
            if self.want_lsend:
                self.dlsend_prototype = (
                    "void dlsend_%(name)s(struct conn_list *dest%(extra_send_args3)s)"
                    % self.__dict__
                )

        # create cap variants
        all_caps = {}
        for f in self.fields:
            if f.add_cap:
                all_caps[f.add_cap] = 1
            if f.remove_cap:
                all_caps[f.remove_cap] = 1

        all_caps = all_caps.keys()
        choices = get_choices(all_caps)
        self.variants = []
        for i, poscaps in enumerate(choices):
            negcaps = all_caps - set(poscaps)
            fields = []
            for field in self.fields:
                if not field.add_cap and not field.remove_cap:
                    fields.append(field)
                elif field.add_cap and field.add_cap in poscaps:
                    fields.append(field)
                elif field.remove_cap and field.remove_cap in negcaps:
                    fields.append(field)
            no = i + 100

            self.variants.append(
                Variant(poscaps, negcaps, "%s_%d" % (self.name, no), fields, self, no)
            )

    def short_name(self):
        """Returns the "short" name of the packet, i.e. without the packet_ prefix."""
        return self.name.replace("packet_", "")

    # Returns a code fragment which contains the struct for this packet.

    def get_struct(self):
        intro = "struct %(name)s {\n" % self.__dict__
        extro = "};\n\n"

        body = ""
        for field in self.key_fields + self.other_fields:
            body = body + "  %s;\n" % field.get_declar()
        if not body:
            body = "  char __dummy;			/* to avoid malloc(0); */\n"
        return intro + body + extro

    # '''

    # Returns a code fragment which represents the prototypes of the
    # send and receive functions for the header file.
    def get_prototypes(self):
        result = self.send_prototype + ";\n"
        if self.want_lsend:
            result = result + self.lsend_prototype + ";\n"
        if self.want_dsend:
            result = result + self.dsend_prototype + ";\n"
            if self.want_lsend:
                result = result + self.dlsend_prototype + ";\n"
        return result + "\n"

    # See Field.get_dict
    def get_dict(self, params):
        result = self.__dict__.copy()
        result.update(params)
        return result

    def get_send(self):
        if self.no_packet:
            func = "no_packet"
            args = ""
        elif self.want_force:
            func = "force_to_send"
            args = ", packet, force_to_send"
        else:
            func = "packet"
            args = ", packet"

        return """%(send_prototype)s
{
  if (!pc->used) {
    qCritical("WARNING: trying to send data to the closed connection %%s",
              conn_description(pc));
    return -1;
  }
  fc_assert_ret_val_msg(pc->phs.handlers->send[%(type)s].%(func)s != NULL, -1,
                        "Handler for %(type)s not installed");
  return pc->phs.handlers->send[%(type)s].%(func)s(pc%(args)s);
}

""" % self.get_dict(
            vars()
        )

    def get_variants(self):
        result = ""
        for v in self.variants:
            if v.delta:
                result = result + v.get_hash()
                result = result + v.get_cmp()
                result = result + v.get_bitvector()
            result = result + v.get_receive()
            result = result + v.get_send()
        return result

    # Returns a code fragment which is the implementation of the
    # lsend function.
    def get_lsend(self):
        if not self.want_lsend:
            return ""
        return (
            """%(lsend_prototype)s
{
  conn_list_iterate(dest, pconn) {
    send_%(name)s(pconn%(extra_send_args2)s);
  } conn_list_iterate_end;
}

"""
            % self.__dict__
        )

    # Returns a code fragment which is the implementation of the
    # dsend function.
    def get_dsend(self):
        if not self.want_dsend:
            return ""
        fill = "\n".join(map(lambda x: x.get_fill(), self.fields))
        return """%(dsend_prototype)s
{
  struct %(name)s packet, *real_packet = &packet;

%(fill)s

  return send_%(name)s(pc, real_packet);
}

""" % self.get_dict(
            vars()
        )

    # Returns a code fragment which is the implementation of the
    # dlsend function.
    def get_dlsend(self):
        if not (self.want_lsend and self.want_dsend):
            return ""
        fill = "\n".join(map(lambda x: x.get_fill(), self.fields))
        return """%(dlsend_prototype)s
{
  struct %(name)s packet, *real_packet = &packet;

%(fill)s

  lsend_%(name)s(dest, real_packet);
}

""" % self.get_dict(
            vars()
        )


# Returns a code fragment which is the implementation of the
# packet_functional_capability string.


def get_packet_functional_capability(packets):
    all_caps = {}
    for p in packets:
        for f in p.fields:
            if f.add_cap:
                all_caps[f.add_cap] = 1
            if f.remove_cap:
                all_caps[f.remove_cap] = 1
    return """
extern "C" const char *const packet_functional_capability = "%s";
""" % " ".join(
        all_caps.keys()
    )


# Returns a code fragment which is the implementation of the
# delta_stats_report() function.


def get_report(packets):
    if not generate_stats:
        return "void delta_stats_report() {}\n\n"

    intro = """
void delta_stats_report() {
  int i;

"""
    extro = "}\n\n"
    body = ""

    for p in packets:
        body = body + p.get_report_part()
    return intro + body + extro


# Returns a code fragment which is the implementation of the
# delta_stats_reset() function.


def get_reset(packets):
    if not generate_stats:
        return "void delta_stats_reset() {}\n\n"
    intro = """
void delta_stats_reset() {
"""
    extro = "}\n\n"
    body = ""

    for p in packets:
        body = body + p.get_reset_part()
    return intro + body + extro


# Returns a code fragment which is the implementation of the
# packet_name() function.


def get_packet_name(packets):
    intro = """const char *packet_name(enum packet_type type)
{
  static const char *const names[PACKET_LAST] = {
"""

    mapping = {}
    for p in packets:
        mapping[p.type_number] = p
    msorted = list(mapping.keys())
    msorted.sort()

    last = -1
    body = ""
    for n in msorted:
        for i in range(last + 1, n):
            body = body + '    "unknown",\n'
        body = body + '    "%s",\n' % mapping[n].type
        last = n

    extro = """  };

  return (type < PACKET_LAST ? names[type] : "unknown");
}

"""
    return intro + body + extro


# Returns a code fragment which is the implementation of the
# packet_has_game_info_flag() function.


def get_packet_has_game_info_flag(packets):
    intro = """bool packet_has_game_info_flag(enum packet_type type)
{
  static const bool flag[PACKET_LAST] = {
"""

    mapping = {}
    for p in packets:
        mapping[p.type_number] = p
    msorted = list(mapping.keys())
    msorted.sort()

    last = -1
    body = ""
    for n in msorted:
        for i in range(last + 1, n):
            body = body + "    false,\n"
        if mapping[n].is_info != "game":
            body = body + "    false, /* %s */\n" % mapping[n].type
        else:
            body = body + "    true, /* %s */\n" % mapping[n].type
        last = n

    extro = """  };

  return (type < PACKET_LAST ? flag[type] : false);
}

"""
    return intro + body + extro


# Returns a code fragment which is the implementation of the
# packet_handlers_fill_initial() function.


def get_packet_handlers_fill_initial(packets):
    intro = """void packet_handlers_fill_initial(struct packet_handlers *phandlers)
{
"""
    all_caps = {}
    for p in packets:
        for f in p.fields:
            if f.add_cap:
                all_caps[f.add_cap] = 1
            if f.remove_cap:
                all_caps[f.remove_cap] = 1
    for cap in all_caps.keys():
        intro = (
            intro
            + """  fc_assert_msg(has_capability("%s", our_capability),
                "Packets have support for unknown '%s' capability!");
"""
            % (cap, cap)
        )

    sc_packets = []
    cs_packets = []
    unrestricted = []
    for p in packets:
        if len(p.variants) == 1:
            # Packets with variants are correctly handled in
            # packet_handlers_fill_capability(). They may remain without
            # handler at connecting time, because it would be anyway wrong
            # to use them before the network capability string would be
            # known.
            if len(p.dirs) == 1 and p.dirs[0] == "sc":
                sc_packets.append(p)
            elif len(p.dirs) == 1 and p.dirs[0] == "cs":
                cs_packets.append(p)
            else:
                unrestricted.append(p)

    body = ""
    for p in unrestricted:
        body = (
            body
            + """  %(send_handler)s
  %(receive_handler)s
"""
            % p.variants[0].__dict__
        )
    body = (
        body
        + """  if (is_server()) {
"""
    )
    for p in sc_packets:
        body = (
            body
            + """    %(send_handler)s
"""
            % p.variants[0].__dict__
        )
    for p in cs_packets:
        body = (
            body
            + """    %(receive_handler)s
"""
            % p.variants[0].__dict__
        )
    body = (
        body
        + """  } else {
"""
    )
    for p in cs_packets:
        body = (
            body
            + """    %(send_handler)s
"""
            % p.variants[0].__dict__
        )
    for p in sc_packets:
        body = (
            body
            + """    %(receive_handler)s
"""
            % p.variants[0].__dict__
        )

    extro = """  }
}

"""
    return intro + body + extro


# Returns a code fragment which is the implementation of the
# packet_handlers_fill_capability() function.


def get_packet_handlers_fill_capability(packets):
    intro = """void packet_handlers_fill_capability(struct packet_handlers *phandlers,
                                     const char *capability)
{
"""

    sc_packets = []
    cs_packets = []
    unrestricted = []
    for p in packets:
        if len(p.variants) > 1:
            if len(p.dirs) == 1 and p.dirs[0] == "sc":
                sc_packets.append(p)
            elif len(p.dirs) == 1 and p.dirs[0] == "cs":
                cs_packets.append(p)
            else:
                unrestricted.append(p)

    body = ""
    for p in unrestricted:
        body = body + "  "
        for v in p.variants:
            body = (
                body
                + """if (%(condition)s) {
    %(log_macro)s("%(type)s: using variant=%(no)s cap=%%s", capability);
    %(send_handler)s
    %(receive_handler)s
  } else """
                % v.__dict__
            )
        body = (
            body
            + """{
    qCritical("Unknown %(type)s variant for cap %%s", capability);
  }
"""
            % v.__dict__
        )
    if len(cs_packets) > 0 or len(sc_packets) > 0:
        body = (
            body
            + """  if (is_server()) {
"""
        )
        for p in sc_packets:
            body = body + "    "
            for v in p.variants:
                body = (
                    body
                    + """if (%(condition)s) {
      %(log_macro)s("%(type)s: using variant=%(no)s cap=%%s", capability);
      %(send_handler)s
    } else """
                    % v.__dict__
                )
            body = (
                body
                + """{
      qCritical("Unknown %(type)s variant for cap %%s", capability);
    }
"""
                % v.__dict__
            )
        for p in cs_packets:
            body = body + "    "
            for v in p.variants:
                body = (
                    body
                    + """if (%(condition)s) {
      %(log_macro)s("%(type)s: using variant=%(no)s cap=%%s", capability);
      %(receive_handler)s
    } else """
                    % v.__dict__
                )
            body = (
                body
                + """{
      qCritical("Unknown %(type)s variant for cap %%s", capability);
    }
"""
                % v.__dict__
            )
        body = (
            body
            + """  } else {
"""
        )
        for p in cs_packets:
            body = body + "    "
            for v in p.variants:
                body = (
                    body
                    + """if (%(condition)s) {
      %(log_macro)s("%(type)s: using variant=%(no)s cap=%%s", capability);
      %(send_handler)s
    } else """
                    % v.__dict__
                )
            body = (
                body
                + """{
      qCritical("Unknown %(type)s variant for cap %%s", capability);
    }
"""
                % v.__dict__
            )
        for p in sc_packets:
            body = body + "    "
            for v in p.variants:
                body = (
                    body
                    + """if (%(condition)s) {
      %(log_macro)s("%(type)s: using variant=%(no)s cap=%%s", capability);
      %(receive_handler)s
    } else """
                    % v.__dict__
                )
            body = (
                body
                + """{
      qCritical("Unknown %(type)s variant for cap %%s", capability);
    }
"""
                % v.__dict__
            )
        body = (
            body
            + """  }
"""
        )

    extro = """}
"""
    return intro + body + extro


# Returns a code fragment which is the declartion of
# "enum packet_type".


def get_enum_packet(packets):
    intro = "enum packet_type {\n"

    mapping = {}
    for p in packets:
        if p.type_number in mapping:
            print(p.name, mapping[p.type_number].name)
            assert 0
        mapping[p.type_number] = p
    msorted = list(mapping.keys())
    msorted.sort()

    last = -1
    body = ""
    for i in msorted:
        p = mapping[i]
        if i != last + 1:
            line = "  %s = %d," % (p.type, i)
        else:
            line = "  %s," % (p.type)

        if (i % 10) == 0:
            line = "%-40s /* %d */" % (line, i)
        body = body + line + "\n"

        last = i
    extro = """
  PACKET_LAST  /* leave this last */
};

"""
    return intro + body + extro


def parse_packet_definitions(content: str) -> str:
    """
    Parses the content of the packets.def file describing the network protocol.
    Returns the list of packets.
    """

    # Remove comments
    content = re.sub(r"/\*(.|\n)*?\*/", "", content)  # C-style
    content = re.sub(r"#.*\n", "\n", content)  # Python-style
    content = re.sub(r"//.*\n", "\n", content)  # C++-style
    content = re.sub(r"\s+\n", "\n", content)  # Empty lines

    # Parse types
    lines = content.splitlines()
    remaining_lines = []
    types = []
    for line in lines:
        mo = re.search("^type\s+(\S+)\s*=\s*(.+)\s*$", line)
        if mo:
            types.append(Type(mo.group(1), mo.group(2)))
        else:
            remaining_lines.append(line)

    # Parse packets
    packets = []
    while remaining_lines:
        index = remaining_lines.index("end")
        definition = remaining_lines[:index]
        packets.append(Packet("\n".join(definition), types))
        remaining_lines = remaining_lines[index + 1 :]

    return packets


def write_disclaimer(output: io.TextIOWrapper) -> None:
    """
    Add a warning that the file output file was generated.
    """
    output.write(
        """
 /****************************************************************************
 *                       THIS FILE WAS GENERATED                             *
 * Script: common/generate_packets.py                                        *
 * Input:  common/networking/packets.def                                     *
 *                       DO NOT CHANGE THIS FILE                             *
 ****************************************************************************/
"""
    )


def write_common_header(packets: list[Packet], output: io.TextIOWrapper) -> None:
    """
    Writes the contents of packets_gen.h to the output stream.
    """

    write_disclaimer(output)
    output.write(
        """
#pragma once

// common
#include "actions.h"
#include "disaster.h"
#include "unit.h"

// common/aicore
#include "cm.h"

"""
    )

    # write structs
    for packet in packets:
        output.write(packet.get_struct())

    output.write(get_enum_packet(packets))

    # write function prototypes
    for packet in packets:
        output.write(packet.get_prototypes())
    output.write(
        """
void delta_stats_report();
void delta_stats_reset();
"""
    )


def write_common_source(packets: list[Packet], output: io.TextIOWrapper) -> None:
    """
    Writes the contents of packets_gen.cpp to the output stream.
    """

    write_disclaimer(output)
    output.write(
        """
#include "packets.h"

// utility
#include "bitvector.h"
#include "capability.h"
#include "fc_config.h"
#include "genhash.h"
#include "log.h"
#include "support.h"

// common
#include "capstr.h"
#include "connection.h"
#include "dataio.h"
#include "game.h"

#include <string.h>
"""
    )
    output.write(get_packet_functional_capability(packets))
    output.write(
        """
static genhash_val_t hash_const(const void *vkey)
{
  return 0;
}

static bool cmp_const(const void *vkey1, const void *vkey2)
{
  return true;
}
"""
    )

    if generate_stats:
        output.write(
            """
static int stats_total_sent;

"""
        )

    if generate_stats:
        # write stats
        for packet in packets:
            output.write(packet.get_stats())
        # write report()
    output.write(get_report(packets))
    output.write(get_reset(packets))

    output.write(get_packet_name(packets))
    output.write(get_packet_has_game_info_flag(packets))

    # write hash, cmp, send, receive
    for packet in packets:
        output.write(packet.get_variants())
        output.write(packet.get_send())
        output.write(packet.get_lsend())
        output.write(packet.get_dsend())
        output.write(packet.get_dlsend())

    output.write(get_packet_handlers_fill_initial(packets))
    output.write(get_packet_handlers_fill_capability(packets))


def write_client_header(packets: list[Packet], output: io.TextIOWrapper) -> None:
    """
    Writes the contents of packhand_gen.h to the output stream.
    """

    write_disclaimer(output)
    output.write(
        """
#pragma once

// utility
#include "shared.h"

// common
#include "packets.h"

bool client_handle_packet(enum packet_type type, const void *packet);

"""
    )
    for packet in packets:
        if "sc" not in packet.dirs:
            continue

        if packet.handle_via_packet:
            output.write(f"struct {packet.name};\n")
            output.write(
                f"void handle_{packet.short_name()}(const {packet.name} *packet);\n"
            )
        else:
            args = map(lambda x: x.get_handle_type() + x.name, packet.fields)
            args = ", ".join(args)
            output.write(f"void handle_{packet.short_name()}({args});\n")


def write_client_source(packets: list[Packet], output: io.TextIOWrapper) -> None:
    """
    Writes the contents of packhand_gen.cpp to the output stream.
    """

    write_disclaimer(output)
    output.write(
        """
#include "packhand_gen.h"

// utility
#include "fc_config.h"

// common
#include "packets.h"

bool client_handle_packet(enum packet_type type, const void *packet)
{
  switch (type) {
"""
    )
    for packet in packets:
        if "sc" not in packet.dirs:
            continue
        if packet.no_handle:
            continue

        packet_cast = f"static_cast<const {packet.name} *>(packet)"

        if packet.handle_via_packet:
            args = packet_cast
        else:
            # static_cast(packet)->a, static_cast(packet)->b, ...
            args = map(lambda field: packet_cast + "->" + field.name, packet.fields)
            args = ",\n      ".join(args)
            if args:
                args = "\n      " + args

        output.write(
            f"""  case {packet.type}:
    handle_{packet.short_name()}({args});
    return true;

"""
        )
    output.write(
        """  default:
    return false;
  }
}
"""
    )


def write_server_header(packets: list[Packet], output: io.TextIOWrapper) -> None:
    """
    Writes the contents of hand_gen.h to the output stream.
    """

    write_disclaimer(output)
    output.write(
        """
#pragma once

// utility
#include "shared.h"

// common
#include "fc_types.h"
#include "packets.h"

struct connection;

bool server_handle_packet(enum packet_type type, const void *packet,
                          struct player *pplayer, struct connection *pconn);

"""
    )

    for packet in packets:
        if not "cs" in packet.dirs:
            continue
        if packet.no_handle:
            continue

        handle_fn_name = f"handle_{packet.short_name()}"
        if packet.handle_via_packet:
            output.write(f"struct {packet.name};\n")
            if packet.handle_per_conn:
                output.write(
                    f"void {handle_fn_name}(connection *pc, const {packet.name} *packet);\n"
                )
            else:
                output.write(
                    f"void {handle_fn_name}(player *pc, const {packet.name} *packet);\n"
                )
        else:
            args = map(lambda field: field.get_handle_type() + field.name, packet.fields)
            args = ", ".join(args)
            if args:
                args = ", " + args

            if packet.handle_per_conn:
                output.write(f"void {handle_fn_name}(connection *pc{args});\n")
            else:
                output.write(f"void {handle_fn_name}(player *pplayer{args});\n")


def write_server_source(packets: list[Packet], output: io.TextIOWrapper) -> None:
    """
    Writes the contents of hand_gen.cpp to the output stream.
    """

    write_disclaimer(output)
    output.write(
        """
#include "hand_gen.h"

// utility
#include "fc_config.h"

// common
#include "packets.h"

bool server_handle_packet(enum packet_type type, const void *packet,
                          struct player *pplayer, struct connection *pconn)
{
  switch (type) {
"""
    )
    for packet in packets:
        if "cs" not in packet.dirs:
            continue
        if packet.no_handle:
            continue
        a = packet.name[len("packet_") :]
        # python doesn't need comments :D
        c = "((const struct %s *)packet)->" % packet.name
        d = "(static_cast<const struct {0}*>(packet))".format(packet.name)
        b = []
        for x in packet.fields:
            y = "%s%s" % (c, x.name)
            if x.dataio_type == "worklist":
                y = "&" + y
            b.append(y)
        b = ",\n      ".join(b)
        if b:
            b = ",\n      " + b

        if packet.handle_via_packet:
            if packet.handle_per_conn:
                # args="pconn, packet"
                args = "pconn, " + d
            else:
                args = "pplayer," + d

        else:
            if packet.handle_per_conn:
                args = "pconn" + b
            else:
                args = "pplayer" + b

        output.write(
            """  case %s:
    handle_%s(%s);
    return true;

"""
            % (packet.type, a, args)
        )
    output.write(
        """  default:
    return false;
  }
}
"""
    )


def _main(input_path: str, mode: str, header: str, source: str) -> None:
    """
    Parses packet definitions from input_path, then writes a header and the
    corresponding source file. "mode" controls which pair of files is produced,
    and can be one of "common", "client", or "server".
    """

    writers = {
        "common": (write_common_header, write_common_source),
        "client": (write_client_header, write_client_source),
        "server": (write_server_header, write_server_source),
    }

    if not mode in writers:
        raise ValueError(mode)

    # Parse input
    with open(input_path, encoding="utf-8") as in_file:
        packets = parse_packet_definitions(in_file.read())

    # Write
    write_header, write_source = writers[mode]
    with open(header, "w", encoding="utf-8") as out_file:
        write_header(packets, out_file)

    with open(source, "w", encoding="utf-8") as out_file:
        write_source(packets, out_file)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--mode",
        choices=("common", "client", "server"),
        help="What to generate (common, client, or server code)",
    )
    parser.add_argument("packets", help="File with packet definitions")
    parser.add_argument("header", help="Path to the header file to produce")
    parser.add_argument("source", help="Path to the source file to produce")
    args = parser.parse_args()

    _main(args.packets, args.mode, args.header, args.source)
