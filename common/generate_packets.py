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
import itertools as it
import re
import typing

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


def indent(prefix, string):
    """
    Prepends `prefix` to every line in `string`.
    """

    lines = string.split("\n")
    lines = map(lambda x: prefix + x, lines)
    return "\n".join(lines)


def get_choices(all_caps):
    """
    Returns all possible combinations of any number of capabilities.
    """

    # Which of the capabilities to take
    caps = it.product([True, False], repeat=len(all_caps))
    # Apply it
    return [list(it.compress(all_caps, selection)) for selection in caps]


def parse_fields(
    line: str, aliases: typing.Mapping[str, str]
) -> list:  # TODO list[Field]
    """
    Parses a line of the form "COORD x, y; flags" and returns a list of Field
    objects. `aliases` is a map of type name: actual type which are used to
    dereference type names.
    """

    if not ";" in line:
        raise ValueError(f"Missing ; in {line}")

    # Split COORD x, y from flags
    declaration, flags = map(str.strip, line.split(sep=";", maxsplit=1))
    # Split flags
    flags = list(map(str.strip, flags.split(sep=","))) if flags else []
    # Split COORD from x, y
    type_name, field_names = declaration.split(maxsplit=1)
    # Split x, y
    field_names = list(map(str.strip, field_names.split(sep=",")))

    # Resolve type
    type_name = aliases.get(type_name, type_name)

    # analyze flags
    flaginfo = {}
    flaginfo["is_key"] = "key" in flags
    if flaginfo["is_key"]:
        flags.remove("key")
    flaginfo["diff"] = "diff" in flags
    if flaginfo["diff"]:
        flags.remove("diff")
    adds = []
    removes = []
    remaining = []
    for i in flags:
        match = re.search(r"^add-cap\((.*)\)$", i)
        if match:
            adds.append(match.group(1))
            continue
        match = re.search(r"^remove-cap\((.*)\)$", i)
        if match:
            removes.append(match.group(1))
            continue
        remaining.append(i)
    flags = remaining
    assert len(flags) == 0, line
    assert len(adds) + len(removes) in [0, 1]

    if adds:
        flaginfo["add_cap"] = adds[0]
    else:
        flaginfo["add_cap"] = ""

    if removes:
        flaginfo["remove_cap"] = removes[0]
    else:
        flaginfo["remove_cap"] = ""

    typeinfo = {}
    match = re.search(r"^(.*)\((.*)\)$", type_name)
    assert match, repr(type_name)
    typeinfo["dataio_type"], typeinfo["struct_type"] = match.groups()

    if typeinfo["struct_type"] == "float":
        match = re.search(r"^(\D+)(\d+)$", typeinfo["dataio_type"])
        assert match
        typeinfo["dataio_type"] = match.group(1)
        typeinfo["float_factor"] = int(match.group(2))

    # analyze fields
    fields = []
    for name in field_names:
        field = {}

        def f(x):
            arr = x.split(":")
            if len(arr) == 1:
                return [x, x, x]

            assert len(arr) == 2
            arr.append("old->" + arr[1])
            arr[1] = "real_packet->" + arr[1]
            return arr

        match = re.search(r"^(.*)\[(.*)\]\[(.*)\]$", name)
        if match:
            field["name"] = match.group(1)
            field["is_array"] = 2
            field["array_size1_d"], field["array_size1_u"], field["array_size1_o"] = f(
                match.group(2)
            )
            field["array_size2_d"], field["array_size2_u"], field["array_size2_o"] = f(
                match.group(3)
            )
        else:
            match = re.search(r"^(.*)\[(.*)\]$", name)
            if match:
                field["name"] = match.group(1)
                field["is_array"] = 1
                field["array_size_d"], field["array_size_u"], field["array_size_o"] = f(
                    match.group(2)
                )
            else:
                field["name"] = name
                field["is_array"] = 0
        fields.append(Field(field, typeinfo, flaginfo))

    return fields


class Field:
    """
    Class for a field (part of a packet). It has a name, several types, flags
    and some other attributes.
    """

    def __init__(self, fieldinfo, typeinfo, flaginfo):
        for i in fieldinfo, typeinfo, flaginfo:
            self.__dict__.update(i)
        self.is_struct = re.search("^struct.*", self.struct_type)

    def get_handle_type(self):
        if self.dataio_type == "string" or self.dataio_type == "estring":
            return "const char *"
        if self.dataio_type == "worklist":
            return f"const {self.struct_type} *"
        if self.is_array:
            return f"const {self.struct_type} *"
        return self.struct_type + " "

    def get_declar(self):
        """
        Returns code which is used in the declaration of the field in the packet
        struct.
        """

        if self.is_array == 2:
            return f"{self.struct_type} {self.name}[{self.array_size1_d}][{self.array_size2_d}]"
        if self.is_array:
            return f"{self.struct_type} {self.name}[{self.array_size_d}]"
        else:
            return f"{self.struct_type} {self.name}"

    def get_fill(self):
        """
        Returns code which copies the arguments of the direct send functions in
        the packet struct.
        """

        if self.dataio_type == "worklist":
            return f"  worklist_copy(&real_packet->{self.name}, {self.name});"
        if self.is_array == 0:
            return f"  real_packet->{self.name} = {self.name};"
        if self.dataio_type in ("string", "estring"):
            return f"  sz_strlcpy(real_packet->{self.name}, {self.name});"
        if self.is_array == 1:
            tmp = f"real_packet->{self.name}[i] = {self.name}[i]"
            return f"""  {{
    int i;

    for (i = 0; i < {self.array_size_u}; i++) {{
      {tmp};
    }}
  }}"""

        assert False

    def get_cmp(self):
        """
        Returns code which sets "differ" by comparing the field instances of
        "old" and "real_packet".
        """

        if self.dataio_type == "memory":
            return f"  differ = (memcmp(old->{self.name}, real_packet->{self.name}, {self.array_size_d}) != 0);"
        if self.dataio_type == "bitvector":
            return (
                f"  differ = !BV_ARE_EQUAL(old->{self.name}, real_packet->{self.name});"
            )
        if self.dataio_type in ["string", "estring"] and self.is_array == 1:
            return (
                f"  differ = (strcmp(old->{self.name}, real_packet->{self.name}) != 0);"
            )
        if self.dataio_type == "cm_parameter":
            return f"  differ = (&old->{self.name} != &real_packet->{self.name});"
        if self.is_struct and self.is_array == 0:
            return f"  differ = !are_{self.dataio_type}s_equal(&old->{self.name}, &real_packet->{self.name});"
        if not self.is_array:
            return f"  differ = (old->{self.name} != real_packet->{self.name});"

        sizes = None, None
        if self.dataio_type in ("string", "estring"):
            c = f"strcmp(old->{self.name}[i], real_packet->{self.name}[i]) != 0"
            sizes = self.array_size1_o, self.array_size1_u
        elif self.is_struct:
            c = f"!are_{self.dataio_type}s_equal(&old->{self.name}[i], &real_packet->{self.name}[i])"
            sizes = self.array_size_o, self.array_size_u
        else:
            c = f"old->{self.name}[i] != real_packet->{self.name}[i]"
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

    def get_cmp_wrapper(self, index):
        """
        Returns a code fragment which updates the bit of the this field in the
        "fields" bitvector. The bit is either a "content-differs" bit or (for
        bools which gets folded in the header) the actual value of the bool.
        """

        if fold_bool_into_header and self.struct_type == "bool" and not self.is_array:
            return f"""{self.get_cmp()}
  if (differ) {{
    different++;
  }}
  if (packet->{self.name}) {{
    BV_SET(fields, {index});
  }}

"""

        return f"""{self.get_cmp()}
  if (differ) {{
    different++;
    BV_SET(fields, {index});
  }}

"""

    def get_put_wrapper(self, packet, index, deltafragment):
        """
        Returns a code fragment which will put this field if the content has
        changed. Does nothing for bools-in-header.
        """

        if fold_bool_into_header and self.struct_type == "bool" and not self.is_array:
            return f"  /* field {index} is folded into the header */\n"
        if packet.gen_log:
            f = f"    {packet.log_macro}(\"  field '{self.name}' has changed\");\n"
        else:
            f = ""

        if packet.gen_stats:
            s = f"    stats_{packet.name}_counters[{index}]++;\n"
        else:
            s = ""

        return f"""  if (BV_ISSET(fields, {index})) {{
{f}{s}  {self.get_put(deltafragment)}
  }}
"""

    def get_put(self, deltafragment):
        """
        Returns code which put this field.
        """

        if self.dataio_type == "bitvector":
            return f"DIO_BV_PUT(&dout, &field_addr, packet->{self.name});"

        if self.struct_type == "float" and not self.is_array:
            return f"  DIO_PUT({self.dataio_type}, &dout, &field_addr, real_packet->{self.name}, {self.float_factor});"

        if self.dataio_type in ["worklist", "cm_parameter"]:
            return f"  DIO_PUT({self.dataio_type}, &dout, &field_addr, &real_packet->{self.name});"

        if self.dataio_type == "memory":
            return f"  DIO_PUT({self.dataio_type}, &dout, &field_addr, &real_packet->{self.name}, {self.array_size_u});"

        arr_types = ["string", "estring", "city_map"]
        if (self.dataio_type in arr_types and self.is_array == 1) or (
            self.dataio_type not in arr_types and self.is_array == 0
        ):
            return f"  DIO_PUT({self.dataio_type}, &dout, &field_addr, real_packet->{self.name});"
        if self.is_struct:
            if self.is_array == 2:
                c = f"DIO_PUT({self.dataio_type}, &dout, &field_addr, &real_packet->{self.name}[i][j]);"
            else:
                c = f"DIO_PUT({self.dataio_type}, &dout, &field_addr, &real_packet->{self.name}[i]);"
        elif self.dataio_type in ("string", "estring"):
            c = f"DIO_PUT({self.dataio_type}, &dout, &field_addr, real_packet->{self.name}[i]);"

        elif self.struct_type == "float":
            if self.is_array == 2:
                c = f"  DIO_PUT({self.dataio_type}, &dout, &field_addr, real_packet->{self.name}[i][j], {self.float_factor});"
            else:
                c = f"  DIO_PUT({self.dataio_type}, &dout, &field_addr, real_packet->{self.name}[i], {self.float_factor});"
        else:
            if self.is_array == 2:
                c = f"DIO_PUT({self.dataio_type}, &dout, &field_addr, real_packet->{self.name}[i][j]);"
            else:
                c = f"DIO_PUT({self.dataio_type}, &dout, &field_addr, real_packet->{self.name}[i]);"

        array_size_u = self.array_size1_u if self.is_array == 2 else self.array_size_u

        if deltafragment and self.diff and self.is_array == 1:
            return f"""
    {{
      int i;

      fc_assert({array_size_u} < 255);

      for (i = 0; i < {array_size_u}; i++) {{
        if (old->{self.name}[i] != real_packet->{self.name}[i]) {{
          DIO_PUT(uint8, &dout, &field_addr, i);

          {c}
        }}
      }}
      DIO_PUT(uint8, &dout, &field_addr, 255);

    }}"""
        if self.is_array == 2 and self.dataio_type not in ("string", "estring"):
            return f"""
    {{
      int i, j;

      for (i = 0; i < {self.array_size1_u}; i++) {{
        for (j = 0; j < {self.array_size2_u}; j++) {{
          {c}
        }}
      }}
    }}"""

        return f"""
    {{
      int i;

      for (i = 0; i < {array_size_u}; i++) {{
        {c}
      }}
    }}"""

    def get_get_wrapper(self, packet, index, deltafragment):
        """
        Returns a code fragment which will get the field if the "fields"
        bitvector says so.
        """

        get = self.get_get(deltafragment)
        if fold_bool_into_header and self.struct_type == "bool" and not self.is_array:
            return f"  real_packet->{self.name} = BV_ISSET(fields, {index});\n"
        get = indent("    ", get)
        log_macro = packet.log_macro
        if packet.gen_log:
            f = f"    {packet.log_macro}(\"  got field '{self.name}'\");\n"
        else:
            f = ""
        return f"""  if (BV_ISSET(fields, {index})) {{
{f}{get}
  }}
"""

    def get_get(self, deltafragment):
        """
        Returns code which get this field.
        """

        if self.struct_type == "float" and not self.is_array:
            return f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, &real_packet->{self.name}, {self.float_factor})) {{
  RECEIVE_PACKET_FIELD_ERROR({self.name});
}}"""
        if self.dataio_type == "bitvector":
            return f"""if (!DIO_BV_GET(&din, &field_addr, real_packet->{self.name})) {{
  RECEIVE_PACKET_FIELD_ERROR({self.name});
}}"""
        if self.dataio_type in ["string", "estring", "city_map"] and self.is_array != 2:
            return f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, real_packet->{self.name}, sizeof(real_packet->{self.name}))) {{
  RECEIVE_PACKET_FIELD_ERROR({self.name});
}}"""
        if self.is_struct and self.is_array == 0:
            return f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, &real_packet->{self.name})) {{
  RECEIVE_PACKET_FIELD_ERROR({self.name});
}}"""
        if not self.is_array:
            if self.struct_type in ["int", "bool"]:
                return f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, &real_packet->{self.name})) {{
  RECEIVE_PACKET_FIELD_ERROR({self.name});
}}"""

            return f"""{{
  int readin;

  if (!DIO_GET({self.dataio_type}, &din, &field_addr, &readin)) {{
    RECEIVE_PACKET_FIELD_ERROR({self.name});
  }}
  real_packet->{self.name} = static_cast<decltype(real_packet->{self.name})>(readin);
}}"""

        if self.is_struct:
            if self.is_array == 2:
                c = f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, &real_packet->{self.name}[i][j])) {{
      RECEIVE_PACKET_FIELD_ERROR({self.name});
    }}"""
            else:
                c = f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, &real_packet->{self.name}[i])) {{
      RECEIVE_PACKET_FIELD_ERROR({self.name});
    }}"""
        elif self.dataio_type == "string" or self.dataio_type == "estring":
            c = f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, real_packet->{self.name}[i], sizeof(real_packet->{self.name}[i]))) {{
      RECEIVE_PACKET_FIELD_ERROR({self.name});
    }}"""
        elif self.struct_type == "float":
            if self.is_array == 2:
                c = f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, &real_packet->{self.name}[i][j], {float_factor})) {{
      RECEIVE_PACKET_FIELD_ERROR({self.name});
    }}"""
            else:
                c = f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, &real_packet->{self.name}[i], {self.float_factor})) {{
      RECEIVE_PACKET_FIELD_ERROR({self.name});
    }}"""
        elif self.is_array == 2:
            if self.struct_type in ["int", "bool"]:
                c = f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, &real_packet->{self.name}[i][j])) {{
      RECEIVE_PACKET_FIELD_ERROR({self.name});
    }}"""
            else:
                c = f"""{{
      int readin;

      if (!DIO_GET({self.dataio_type}, &din, &field_addr, &readin)) {{
        RECEIVE_PACKET_FIELD_ERROR({self.name});
      }}
      real_packet->{self.name}[i][j] = readin;
    }}"""
        elif self.struct_type in ["int", "bool"]:
            c = f"""if (!DIO_GET({self.dataio_type}, &din, &field_addr, &real_packet->{self.name}[i])) {{
      RECEIVE_PACKET_FIELD_ERROR({self.name});
    }}"""
        else:
            c = f"""{{
      int readin;

      if (!DIO_GET({self.dataio_type}, &din, &field_addr, &readin)) {{
        RECEIVE_PACKET_FIELD_ERROR({self.name});
      }}
      real_packet->{self.name}[i] = readin;
    }}"""

        if self.is_array == 2:
            array_size_u = self.array_size1_u
            array_size_d = self.array_size1_d
        else:
            array_size_u = self.array_size_u
            array_size_d = self.array_size_d

        if not self.diff or self.dataio_type == "memory":
            if array_size_u != array_size_d:
                extra = f"""
  if ({array_size_u} > {array_size_d}) {{
    RECEIVE_PACKET_FIELD_ERROR({self.name}, ": truncation array");
  }}"""
            else:
                extra = ""
            if self.dataio_type == "memory":
                return f"""{extra}
  if (!DIO_GET({self.dataio_type}, &din, &field_addr, real_packet->{self.name}, {array_size_u})) {{
    RECEIVE_PACKET_FIELD_ERROR({self.name});
  }}"""
            if self.is_array == 2 and self.dataio_type not in ("string", "estring"):
                return f"""
{{
  int i, j;

{extra}
  for (i = 0; i < {self.array_size1_u}; i++) {{
    for (j = 0; j < {self.array_size2_u}; j++) {{
      {c}
    }}
  }}
}}"""
            else:
                return f"""
{{
  int i;

{extra}
  for (i = 0; i < {array_size_u}; i++) {{
    {c}
  }}
}}"""
        elif deltafragment and self.diff and self.is_array == 1:
            return f"""
{{
int count;

for (count = 0;; count++) {{
  int i;

  if (!DIO_GET(uint8, &din, &field_addr, &i)) {{
    RECEIVE_PACKET_FIELD_ERROR({self.name});
  }}
  if (i == 255) {{
    break;
  }}
  if (i >= {array_size_u}) {{
    RECEIVE_PACKET_FIELD_ERROR({self.name},
                               \": unexpected value %d \"
                               \"(> {array_size_u}) in array diff\",
                               i);
  }} else {{
    {c}
  }}
}}
}}"""
        else:
            return f"""
{{
  int i;

  for (i = 0; i < {array_size_u}; i++) {{
    {c}
  }}
}}"""


class Variant:
    """
    Class which represents a capability variant.
    """

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
                f", const {self.packet_name} *packet" + self.extra_send_args
            )
            self.extra_send_args2 = ", packet" + self.extra_send_args2

        if self.want_force:
            self.extra_send_args = self.extra_send_args + ", bool force_to_send"
            self.extra_send_args2 = self.extra_send_args2 + ", force_to_send"
            self.extra_send_args3 = self.extra_send_args3 + ", bool force_to_send"

        self.receive_prototype = (
            f"static {self.packet_name} *receive_{self.name}(connection *pc)"
        )
        self.send_prototype = (
            f"static int send_{self.name}(connection *pc{self.extra_send_args})"
        )

        if self.no_packet:
            self.send_handler = f"phandlers->send[{self.type}].no_packet = (int(*)(connection *)) send_{self.name};"
        elif self.want_force:
            self.send_handler = f"phandlers->send[{self.type}].force_to_send = (int(*)(connection *, const void *, bool)) send_{self.name};"
        else:
            self.send_handler = f"phandlers->send[{self.type}].packet = (int(*)(connection *, const void *)) send_{self.name};"
        self.receive_handler = f"phandlers->receive[{self.type}] = (void *(*)(connection *)) receive_{self.name};"

    def get_stats(self):
        """
        Returns a code fragment which contains the declarations of the
        statistical counters of this packet.
        """

        names = map(lambda x: '"' + x.name + '"', self.other_fields)
        names = ", ".join(names)

        return f"""static int stats_{self.name}_sent;
static int stats_{self.name}_discarded;
static int stats_{self.name}_counters[{self.bits}];
static char *stats_{self.name}_names[] = {{names}};

"""

    def get_bitvector(self):
        """
        Returns a code fragment which declares the packet specific bitvector.
        Each bit in this bitvector represents one non-key field.
        """

        return f"BV_DEFINE({self.name}_fields, {self.bits});\n"

    def get_report_part(self):
        """
        Returns a code fragment which is the packet specific part of the
        delta_stats_report() function.
        """

        return f"""
  if (stats_{self.name}_sent > 0
      && stats_{self.name}_discarded != stats_{self.name}_sent) {{
    log_test(\"{self.name} %d out of %d got discarded\",
      stats_{self.name}_discarded, stats_{self.name}_sent);
    for (i = 0; i < {self.bits}; i++) {{
      if (stats_{self.name}_counters[i] > 0) {{
        log_test(\"  %4d / %4d: %2d = %s\",
          stats_{self.name}_counters[i],
          (stats_{self.name}_sent - stats_{self.name}_discarded),
          i, stats_{self.name}_names[i]);
      }}
    }}
  }}
"""

    def get_reset_part(self):
        """
        Returns a code fragment which is the packet specific part of the
        delta_stats_reset() function.
        """

        return f"""
  stats_{self.name}_sent = 0;
  stats_{self.name}_discarded = 0;
  memset(stats_{self.name}_counters, 0,
         sizeof(stats_{self.name}_counters));
"""

    def get_hash(self):
        """
        Returns a code fragment which is the implementation of the hash
        function. The hash function is using all key fields.
        """

        if len(self.key_fields) == 0:
            return f"#define hash_{self.name} hash_const\n\n"

        intro = f"""static genhash_val_t hash_{self.name}(const void *vkey)
{{
"""

        body = f"""  const {self.packet_name} *key = (const {self.packet_name} *) vkey;

"""

        keys = list(map(lambda x: "key->" + x.name, self.key_fields))
        if len(keys) == 1:
            a = keys[0]
        elif len(keys) == 2:
            a = f"({keys[0]} << 8) ^ {keys[1]}"
        else:
            assert 0
        body += f"  return {a};\n"
        extro = "}\n\n"
        return intro + body + extro

    def get_cmp(self):
        """
        Returns a code fragment which is the implementation of the cmp function.
        The cmp function is using all key fields. The cmp function is used for
        the hash table.
        """

        if len(self.key_fields) == 0:
            return f"#define cmp_{self.name} cmp_const\n\n"

        intro = f"""static bool cmp_{self.name}(const void *vkey1, const void *vkey2)
{{
"""
        body = f"""  const {self.packet_name} *key1 = (const {self.packet_name} *) vkey1;
  const {self.packet_name} *key2 = (const {self.packet_name} *) vkey2;

"""
        for field in self.key_fields:
            body += f"""  return key1->{field.name} == key2->{field.name};
"""
        extro = "}\n"
        return intro + body + extro

    def get_send(self):
        """
        Returns a code fragment which is the implementation of the send
        function. This is one of the two real functions. So it is rather
        complex to create.
        """

        temp = f"""{self.send_prototype}
{{
<real_packet1><delta_header>  SEND_PACKET_START({self.type});
<faddr><log><report><pre1><body><pre2><post>  SEND_PACKET_END({self.type});
}}

"""
        if self.gen_stats:
            report = f"""
  stats_total_sent++;
  stats_{self.name}_sent++;
"""
        else:
            report = ""
        if self.gen_log:
            log = f'\n  {self.log_macro}("{self.name}: sending info about ({self.keys_format})"{self.keys_arg});\n'
        else:
            log = ""
        if self.want_pre_send:
            pre1 = f"""
  {{
    auto tmp = new {self.packet_name}();

    *tmp = *packet;
    pre_send_{self.packet_name}(pc, tmp);
    real_packet = tmp;
  }}
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
            real_packet1 = f"  const {self.packet_name} *real_packet = packet;\n"
        else:
            real_packet1 = ""

        if not self.no_packet:
            if self.delta:
                if self.want_force:
                    diff = "force_to_send"
                else:
                    diff = "0"
                delta_header = f"""
  {self.name}_fields fields;
  {self.packet_name} *old;
  bool differ;
  genhash **hash = pc->phs.sent + {self.type};
  int different = {diff};
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
                post = f"  post_send_{self.packet_name}(pc, NULL);\n"
            else:
                post = f"  post_send_{self.packet_name}(pc, real_packet);\n"
        else:
            post = ""

        faddr = ""

        for i in range(2):
            for k, v in vars().items():
                if isinstance(v, str):
                    temp = temp.replace(f"<{k}>", v)
        return temp

    def get_delta_send_body(self):
        """
        Helper for get_send()
        """

        intro = f"""
  if (NULL == *hash) {{
    *hash = genhash_new_full(hash_{self.name}, cmp_{self.name},
                             NULL, NULL, NULL, free);
  }}
  BV_CLR_ALL(fields);

  if (!genhash_lookup(*hash, real_packet, (void **) &old)) {{
    old = new {self.packet_name};
    *old = *real_packet;
    genhash_insert(*hash, old, old);
    memset(old, 0, sizeof(*old));
    different = 1;      /* Force to send. */
  }}
"""
        body = ""
        for i in range(len(self.other_fields)):
            field = self.other_fields[i]
            body = body + field.get_cmp_wrapper(i)
        if self.gen_log:
            fl = f'    {self.log_macro}("  no change -> discard");\n'
        else:
            fl = ""
        if self.gen_stats:
            s = f"    stats_{self.name}_discarded++;\n"
        else:
            s = ""

        if self.is_info != "no":
            body += f"""
  if (different == 0) {{
{fl}{s}<pre2>    return 0;
  }}
"""

        body += """
  DIO_BV_PUT(&dout, &field_addr, fields);
"""

        for field in self.key_fields:
            body += field.get_put(1) + "\n"
        body += "\n"

        for i in range(len(self.other_fields)):
            field = self.other_fields[i]
            body += field.get_put_wrapper(self, i, 1)
        body += """
  *old = *real_packet;
"""

        # Cancel some is-info packets.
        for i in self.cancel:
            body += f"""
  hash = pc->phs.sent + {i};
  if (NULL != *hash) {{
    genhash_remove(*hash, real_packet);
  }}
"""

        return intro + body

    def get_receive(self):
        """
        Returns a code fragment which is the implementation of the receive
        function. This is one of the two real functions. So it is rather complex
        to create.
        """

        temp = f"""{self.receive_prototype}
{{
<delta_header>  RECEIVE_PACKET_START({self.packet_name}, real_packet);
<faddr><delta_body1><body1><log><body2><post>  RECEIVE_PACKET_END(real_packet);
}}

"""
        if self.delta:
            delta_header = f"""
  {self.name}_fields fields;
  {self.packet_name} *old;
  genhash **hash = pc->phs.received + {self.type};
"""
            delta_body1 = """
  DIO_BV_GET(&din, &field_addr, fields);
  """
            body1 = ""
            for field in self.key_fields:
                body1 = body1 + indent("  ", field.get_get(1)) + "\n"
            body2 = self.get_delta_receive_body()
        else:
            delta_header = ""
            delta_body1 = ""
            body1 = ""
            for field in self.fields:
                body1 += indent("  ", field.get_get(0)) + "\n"
            body2 = ""
        body1 = body1 + "\n"

        if self.gen_log:
            log = f'  {self.log_macro}("{self.name}: got info about ({self.keys_format})"{self.keys_arg});\n'
        else:
            log = ""

        if self.want_post_recv:
            post = f"  post_receive_{self.packet_name}(pc, real_packet);\n"
        else:
            post = ""

        faddr = ""

        for i in range(2):
            for k, v in vars().items():
                if isinstance(v, str):
                    temp = temp.replace(f"<{k}>", v)
        return temp

    # Helper for get_receive()
    def get_delta_receive_body(self):
        key1 = map(
            lambda x: f"    {x.struct_type} {x.name} = real_packet->{x.name};",
            self.key_fields,
        )
        key2 = map(lambda x: f"    real_packet->{x.name} = {x.name};", self.key_fields)
        key1 = "\n".join(key1)
        key2 = "\n".join(key2)
        if key1:
            key1 = key1 + "\n\n"
        if key2:
            key2 = "\n\n" + key2
        if self.gen_log:
            fl = f'    {self.log_macro}("  no old info");\n'
        else:
            fl = ""
        body = f"""
  if (NULL == *hash) {{
    *hash = genhash_new_full(hash_{self.name}, cmp_{self.name},
                             NULL, NULL, NULL, free);
  }}

  if (genhash_lookup(*hash, real_packet, (void **) &old)) {{
    *real_packet = *old;
  }} else {{
{key1}{fl}    memset(real_packet, 0, sizeof(*real_packet));{key2}
  }}

"""
        for i in range(len(self.other_fields)):
            field = self.other_fields[i]
            body = body + field.get_get_wrapper(self, i, 1)

        extro = f"""
  if (NULL == old) {{
    old = new {self.packet_name};
    *old = *real_packet;
    genhash_insert(*hash, old, old);
  }} else {{
    *old = *real_packet;
  }}
"""

        # Cancel some is-info packets.
        for i in self.cancel:
            extro += f"""
  hash = pc->phs.received + {i};
  if (NULL != *hash) {{
    genhash_remove(*hash, real_packet);
  }}
"""

        return body + extro


class Packet:
    """
    Class representing a packet. A packet contains a list of fields.
    """

    def __init__(self, lines: list[str], aliases: typing.Mapping[str, str]):
        """
        Parses a packet definition from the provided lines.
        """

        self.log_macro = use_log_macro
        self.gen_stats = generate_stats
        self.gen_log = generate_logs

        lines = list(map(str.strip, lines))

        # Parse header: PACKET_A_B = 123; sc, lsend
        header = lines[0]
        del lines[0]

        match = re.search(r"^(\S+)\s*=\s*(\d+)\s*;\s*(.*?)$", header)
        assert match, repr(header)

        self.type = match.group(1)  # PACKET_A_B
        self.name = self.type.lower()  # packet_a_b
        self.type_number = int(match.group(2))  # 123
        assert 0 <= self.type_number <= 0xFFFF

        # sc, lsend
        flags = set(flag.strip() for flag in match.group(3).split(","))

        self.dirs = []

        if "sc" in flags:
            self.dirs.append("sc")
            flags.remove("sc")
        if "cs" in flags:
            self.dirs.append("cs")
            flags.remove("cs")
        assert len(self.dirs) > 0, repr(self.name) + repr(self.dirs)

        # "no" means normal packet
        # "yes" means is-info packet
        # "game" means is-game-info packet
        self.is_info = "no"
        if "is-info" in flags:
            self.is_info = "yes"
            flags.remove("is-info")
        if "is-game-info" in flags:
            self.is_info = "game"
            flags.remove("is-game-info")

        self.want_pre_send = "pre-send" in flags
        if self.want_pre_send:
            flags.remove("pre-send")

        self.want_post_recv = "post-recv" in flags
        if self.want_post_recv:
            flags.remove("post-recv")

        self.want_post_send = "post-send" in flags
        if self.want_post_send:
            flags.remove("post-send")

        self.delta = "no-delta" not in flags
        if not self.delta:
            flags.remove("no-delta")

        self.no_packet = "no-packet" in flags
        if self.no_packet:
            flags.remove("no-packet")

        self.handle_via_packet = "handle-via-packet" in flags
        if self.handle_via_packet:
            flags.remove("handle-via-packet")

        self.handle_per_conn = "handle-per-conn" in flags
        if self.handle_per_conn:
            flags.remove("handle-per-conn")

        self.no_handle = "no-handle" in flags
        if self.no_handle:
            flags.remove("no-handle")

        self.dsend_given = "dsend" in flags
        if self.dsend_given:
            flags.remove("dsend")

        self.want_lsend = "lsend" in flags
        if self.want_lsend:
            flags.remove("lsend")

        self.want_force = "force" in flags
        if self.want_force:
            flags.remove("force")

        self.cancel = []
        removes = []
        remaining = []
        for i in flags:
            match = re.search(r"^cancel\((.*)\)$", i)
            if match:
                self.cancel.append(match.group(1))
                continue
            remaining.append(i)
        flags = remaining

        assert len(flags) == 0, repr(flags)

        self.fields = []
        for i in lines:
            self.fields = self.fields + parse_fields(i, aliases)
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
            self.extra_send_args = f", const {self.name} *packet" + self.extra_send_args
            self.extra_send_args2 = ", packet" + self.extra_send_args2

        if self.want_force:
            self.extra_send_args += ", bool force_to_send"
            self.extra_send_args2 += ", force_to_send"
            self.extra_send_args3 += ", bool force_to_send"

        self.send_prototype = (
            f"int send_{self.name}(connection *pc{self.extra_send_args})"
        )
        if self.want_lsend:
            self.lsend_prototype = (
                f"void lsend_{self.name}(conn_list *dest{self.extra_send_args})"
            )
        if self.want_dsend:
            self.dsend_prototype = (
                f"int dsend_{self.name}(connection *pc{self.extra_send_args3})"
            )
            if self.want_lsend:
                self.dlsend_prototype = (
                    f"void dlsend_{self.name}(conn_list *dest{self.extra_send_args3})"
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

    def get_struct(self):
        """
        Returns a code fragment which contains the struct for this packet.
        """

        intro = f"struct {self.name} {{\n"
        extro = "};\n\n"

        body = ""
        for field in self.key_fields + self.other_fields:
            body += f"  {field.get_declar()};\n"
        return intro + body + extro

    def get_prototypes(self):
        """
        Returns a code fragment which represents the prototypes of the send and
        receive functions for the header file.
        """

        result = self.send_prototype + ";\n"
        if self.want_lsend:
            result = result + self.lsend_prototype + ";\n"
        if self.want_dsend:
            result = result + self.dsend_prototype + ";\n"
            if self.want_lsend:
                result = result + self.dlsend_prototype + ";\n"
        return result + "\n"

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

        return f"""{self.send_prototype}
{{
  if (!pc->used) {{
    qCritical("WARNING: trying to send data to the closed connection %s",
              conn_description(pc));
    return -1;
  }}
  fc_assert_ret_val_msg(pc->phs.handlers->send[{self.type}].{func} != NULL, -1,
                        "Handler for {self.type} not installed");
  return pc->phs.handlers->send[{self.type}].{func}(pc{args});
}}

"""

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

    def get_lsend(self):
        """
        Returns a code fragment which is the implementation of the lsend
        function.
        """

        if not self.want_lsend:
            return ""
        return f"""{self.lsend_prototype}
{{
  conn_list_iterate(dest, pconn) {{
    send_{self.name}(pconn{self.extra_send_args2});
  }} conn_list_iterate_end;
}}

"""

    def get_dsend(self):
        """
        Returns a code fragment which is the implementation of the dsend
        function.
        """

        if not self.want_dsend:
            return ""
        fill = "\n".join(map(lambda x: x.get_fill(), self.fields))
        return f"""{self.dsend_prototype}
{{
  {self.name} packet, *real_packet = &packet;

{fill}

  return send_{self.name}(pc, real_packet);
}}

"""

    def get_dlsend(self):
        """
        Returns a code fragment which is the implementation of the dlsend
        function.
        """

        if not (self.want_lsend and self.want_dsend):
            return ""
        fill = "\n".join(map(lambda x: x.get_fill(), self.fields))
        return f"""{self.dlsend_prototype}
{{
  {self.name} packet, *real_packet = &packet;

{fill}

  lsend_{self.name}(dest, real_packet);
}}

"""


def get_packet_functional_capability(packets):
    """
    Returns a code fragment which is the implementation of the
    packet_functional_capability string.
    """

    all_caps = set()
    for p in packets:
        for f in p.fields:
            if f.add_cap:
                all_caps.add(f.add_cap)
            if f.remove_cap:
                all_caps.add(f.remove_cap)
    capstr = " ".join(all_caps)
    return f"""
extern "C" const char *const packet_functional_capability = "{capstr}";
"""


def get_report(packets):
    """
    Returns a code fragment which is the implementation of the
    delta_stats_report() function.
    """

    if not generate_stats:
        return "void delta_stats_report() {}\n\n"

    intro = """
void delta_stats_report() {
  int i;

"""
    extro = "}\n\n"
    body = ""

    for p in packets:
        body += p.get_report_part()
    return intro + body + extro


def get_reset(packets):
    """
    Returns a code fragment which is the implementation of the
    delta_stats_reset() function.
    """

    if not generate_stats:
        return "void delta_stats_reset() {}\n\n"
    intro = """
void delta_stats_reset() {
"""
    extro = "}\n\n"
    body = ""

    for p in packets:
        body += p.get_reset_part()
    return intro + body + extro


def get_packet_name(packets):
    """
    Returns a code fragment which is the implementation of the  packet_name()
    function.
    """

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
        body = body + f'    "{mapping[n].type}",\n'
        last = n

    extro = """  };

  return (type < PACKET_LAST ? names[type] : "unknown");
}

"""
    return intro + body + extro


def get_packet_has_game_info_flag(packets):
    """
    Returns a code fragment which is the implementation of the
    packet_has_game_info_flag() function.
    """

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
            body += "    false,\n"
        if mapping[n].is_info != "game":
            body += f"    false, /* {mapping[n].type} */\n"
        else:
            body += f"    true, /* {mapping[n].type} */\n"
        last = n

    extro = """  };

  return (type < PACKET_LAST ? flag[type] : false);
}

"""
    return intro + body + extro


def get_packet_handlers_fill_initial(packets):
    """
    Returns a code fragment which is the implementation of the
    packet_handlers_fill_initial() function.
    """

    intro = """void packet_handlers_fill_initial(packet_handlers *phandlers)
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
        intro += f"""  fc_assert_msg(has_capability("{cap}", our_capability),
                "Packets have support for unknown '{cap}' capability!");
"""

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
        body += f"""  {p.variants[0].send_handler}
  {p.variants[0].receive_handler}
"""
    body += """  if (is_server()) {
"""
    for p in sc_packets:
        body += f"""    {p.variants[0].send_handler}
"""
    for p in cs_packets:
        body += f"""    {p.variants[0].receive_handler}
"""
    body += """  } else {
"""
    for p in cs_packets:
        body += f"""    {p.variants[0].send_handler}
"""
    for p in sc_packets:
        body += f"""    {p.variants[0].receive_handler}
"""

    extro = """  }
}

"""
    return intro + body + extro


def get_packet_handlers_fill_capability(packets: list[Packet]) -> str:
    """
    Returns a code fragment which is the implementation of the
    packet_handlers_fill_capability() function.
    """

    intro = """void packet_handlers_fill_capability(packet_handlers *phandlers,
                                     const char *capability)
{
"""

    def variant_conditional(
        prefix: str, packet: Packet, code_func: typing.Callable[Variant, str]
    ) -> str:
        """
        Produces code of the form:

        if (variant1) {
            {code_func(variant1)}
        } else if (variant2) {
            {code_func(variant2)}
        } else {
            log error
        }
        """

        code = ""
        for var in packet.variants:
            code += f"""if ({var.condition}) {{
  {var.log_macro}("{var.type}: using variant={var.no} cap=%s", capability);
  {code_func(var)}
}} else """

        code += f"""{{
  qCritical("Unknown {packet.type} variant for cap %s", capability);
}}"""
        return indent(prefix, code) + "\n"

    sc_packets = []
    cs_packets = []
    unrestricted = []
    for packet in packets:
        if len(packet.variants) > 1:
            if len(packet.dirs) == 1 and packet.dirs[0] == "sc":
                sc_packets.append(packet)
            elif len(packet.dirs) == 1 and packet.dirs[0] == "cs":
                cs_packets.append(packet)
            else:
                unrestricted.append(packet)

    body = ""
    for packet in unrestricted:
        body += variant_conditional(
            "  ", packet, lambda var: var.send_handler + "\n  " + var.receive_handler
        )

    if cs_packets or sc_packets:
        body += "  if (is_server()) {\n"
        for packet in sc_packets:
            body += variant_conditional("    ", packet, lambda var: var.send_handler)

        for packet in cs_packets:
            body += variant_conditional("    ", packet, lambda var: var.receive_handler)

        body += "  } else {\n"
        for packet in cs_packets:
            body += variant_conditional("    ", packet, lambda var: var.send_handler)

        for packet in sc_packets:
            body += variant_conditional("    ", packet, lambda var: var.receive_handler)

        body += "  }\n"

    extro = "}\n"
    return intro + body + extro


def get_enum_packet(packets: list[Packet]) -> str:
    """
    Returns a code fragment which is the declartion of "enum packet_type".
    """

    intro = "enum packet_type {\n"

    mapping = {}
    for packet in packets:
        assert (
            packet.type_number not in mapping
        ), f"same number: {packet.name}/{mapping[packet.type_number].name}"
        mapping[packet.type_number] = packet
    msorted = list(mapping.keys())
    msorted.sort()

    last = -1
    body = ""
    for index in msorted:
        packet = mapping[index]
        if index != last + 1:
            line = f"  {packet.type} = {index},"
        else:
            line = f"  {packet.type},"

        if index % 10 == 0:
            line = f"{line:40s} /* {index} */"

        body += line + "\n"

        last = index
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
    content = re.sub(r"\s+\n", "\n", content).strip()  # Empty lines

    # Parse type aliases
    lines = content.splitlines()
    remaining_lines: list[str] = []
    aliases: typing.Mapping[str, str] = {}
    for line in lines:
        match = re.search(r"^type\s+(\S+)\s*=\s*(.+)\s*$", line)
        if match:
            aliases[match.group(1)] = match.group(2)
        else:
            remaining_lines.append(line)

    # Simplify aliases
    for key in aliases:
        while aliases[key] in aliases:
            aliases[key] = aliases[aliases[key]]

    # Parse packets
    packets = []
    while remaining_lines:
        index = remaining_lines.index("end")
        packets.append(Packet(remaining_lines[:index], aliases))
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
                          player *pplayer, connection *pconn);

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
            args = map(
                lambda field: field.get_handle_type() + field.name, packet.fields
            )
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
                          player *pplayer, connection *pconn)
{
  switch (type) {
"""
    )
    for packet in packets:
        if "cs" not in packet.dirs:
            continue
        if packet.no_handle:
            continue

        cast = f"static_cast<const {packet.name} *>(packet)"

        if packet.handle_via_packet:
            if packet.handle_per_conn:
                # args = "pconn, packet"
                args = "pconn, " + cast
            else:
                args = "pplayer, " + cast

        else:
            args = []
            for field in packet.fields:
                arg = f"{cast}->{field.name}"
                if field.dataio_type == "worklist":
                    arg = "&" + arg
                args.append(arg)
            args = ",\n      ".join(args)
            if args:
                args = ",\n      " + args

            if packet.handle_per_conn:
                args = "pconn" + args
            else:
                args = "pplayer" + args

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
    arguments = parser.parse_args()

    _main(arguments.packets, arguments.mode, arguments.header, arguments.source)
