#!/usr/bin/env python3

# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
# SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

import argparse
import io
import itertools as it
import re
import typing
from textwrap import dedent, indent

# The following parameters change the amount of output.

# generate_stats will generate a large amount of statistics how many
# info packets got discarded and how often a field is transmitted. You
# have to call delta_stats_report to get these.
generate_stats = 0

# generate_logs will generate log calls to debug the delta code.
generate_logs = 1
use_log_macro = "log_packet_detailed"

################# END OF PARAMETERS ####################


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

    capability = None
    capability_add = True
    remaining = []
    for i in flags:
        match = re.search(r"^add-cap\((.*)\)$", i)
        if match:
            capability = match.group(1)
            break
        match = re.search(r"^remove-cap\((.*)\)$", i)
        if match:
            capability = match.group(1)
            capability_add = False
            break
        remaining.append(i)
    flags = remaining
    assert len(flags) == 0, line

    flaginfo["capability"] = capability
    flaginfo["capability_add"] = capability_add

    typeinfo = {}
    match = re.search(r"^(.*)\((.*)\)$", type_name)
    assert match, repr(type_name)
    typeinfo["dataio_type"], typeinfo["struct_type"] = match.groups()

    if typeinfo["struct_type"] == "float":
        match = re.search(r"^(.+)/(\d+)$", typeinfo["dataio_type"])
        assert match
        typeinfo["dataio_type"] = match.group(1)
        typeinfo["float_factor"] = int(match.group(2))

    # analyze fields
    fields = []
    for name in field_names:
        field = {}

        def field_dims(x):
            parts = x.split(":")
            if len(parts) == 1:
                return [x, x, x]

            assert len(parts) == 2
            return [parts[0], f"real_packet->{parts[1]}", f"old->{parts[1]}"]

        match = re.search(r"^(.*)\[(.*)\]\[(.*)\]$", name)
        if match:
            field["name"] = match.group(1)
            field["array_dims"] = 2
            field["array_size1_d"], field["array_size1_u"], field["array_size1_o"] = (
                field_dims(match.group(2))
            )
            field["array_size2_d"], field["array_size2_u"], field["array_size2_o"] = (
                field_dims(match.group(3))
            )
        else:
            match = re.search(r"^(.*)\[(.*)\]$", name)
            if match:
                field["name"] = match.group(1)
                field["array_dims"] = 1
                field["array_size_d"], field["array_size_u"], field["array_size_o"] = (
                    field_dims(match.group(2))
                )
            else:
                field["name"] = name
                field["array_dims"] = 0
        fields.append(Field(field, typeinfo, flaginfo))

    return fields


class Field:
    """
    Class for a field (part of a packet). It has a name, several types, flags
    and some other attributes.
    """

    name: str
    dataio_type: str
    struct_type: str

    float_factor: int
    diff: bool

    array_dims: int
    array_size_d: int
    array_size_o: int
    array_size_u: int
    array_size1_d: int
    array_size1_o: int
    array_size1_u: int
    array_size2_d: int
    array_size2_o: int
    array_size2_u: int

    def __init__(self, fieldinfo, typeinfo, flaginfo):
        for i in fieldinfo, typeinfo, flaginfo:
            self.__dict__.update(i)
        self.is_struct = re.search("^struct.*", self.struct_type)

        if self.capability is None:
            self.condition = None
            return

        cap_name = packet_capability_name(self.capability)
        if self.capability_add:
            self.condition = f"capability & (1 << {cap_name})"
        else:
            self.condition = f"!(capability & (1 << {cap_name}))"

    def get_handle_type(self):
        if self.dataio_type == "string":
            return "const char *"
        if self.dataio_type == "worklist":
            return f"const {self.struct_type} *"
        if self.array_dims:
            return f"const {self.struct_type} *"
        return self.struct_type + " "

    def get_declar(self):
        """
        Returns code which is used in the declaration of the field in the packet
        struct.
        """

        if self.array_dims == 2:
            if self.dataio_type in {"string", "memory"}:
                return f"std::array<{self.struct_type}[{self.array_size2_d}], {self.array_size1_d}> {self.name}"
            else:
                return f"std::array<std::array<{self.struct_type}, {self.array_size2_d}>, {self.array_size1_d}> {self.name}"
        if self.array_dims == 1:
            if self.dataio_type in {"string", "memory"}:
                return f"{self.struct_type} {self.name}[{self.array_size_d}]"
            else:
                return (
                    f"std::array<{self.struct_type}, {self.array_size_d}> {self.name}"
                )
        else:
            return f"{self.struct_type} {self.name}"

    def get_fill(self):
        """
        Returns code which copies the arguments of the direct send functions in
        the packet struct.
        """

        if self.dataio_type == "worklist":
            return f"  worklist_copy(&real_packet->{self.name}, {self.name});"
        if self.array_dims == 0:
            return f"  real_packet->{self.name} = {self.name};"
        if self.dataio_type == "string":
            return f"  sz_strlcpy(real_packet->{self.name}, {self.name});"
        if self.array_dims == 1:
            tmp = f"real_packet->{self.name}[i] = {self.name}[i]"
            return f"""
  for (int i = 0; i < {self.array_size_u}; i++) {{
    {tmp};
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
        if self.dataio_type == "string" and self.array_dims == 1:
            return (
                f"  differ = (strcmp(old->{self.name}, real_packet->{self.name}) != 0);"
            )
        if self.dataio_type == "cm_parameter":
            return f"  differ = (&old->{self.name} != &real_packet->{self.name});"
        if self.is_struct and self.array_dims == 0:
            return f"  differ = !are_{self.dataio_type}s_equal(&old->{self.name}, &real_packet->{self.name});"
        if not self.array_dims:
            return f"  differ = (old->{self.name} != real_packet->{self.name});"

        sizes = None, None
        if self.dataio_type == "string":
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

    def get_cmp_wrapper(self):
        """
        Returns a code fragment which updates the bit of the this field in the
        "fields" bitvector. The bit is either a "content-differs" bit or (for
        bools which gets folded in the header) the actual value of the bool.
        """

        if self.struct_type == "bool" and not self.array_dims:
            return f"""{self.get_cmp()}
  if (differ) {{
    different++;
  }}
  if (packet->{self.name}) {{
    fields.setBit(index);
  }}
  index++;

"""

        return f"""{self.get_cmp()}
  if (differ) {{
    different++;
    fields.setBit(index);
  }}
  index++;

"""

    def get_put_wrapper(self, packet):
        """
        Returns a code fragment which will put this field if the content has
        changed. Does nothing for bools-in-header.
        """

        if self.struct_type == "bool" and not self.array_dims:
            return f"  index++; // {self.name} folded into the header\n"
        if packet.gen_log:
            f = f"    {packet.log_macro}(\"  field '{self.name}' has changed\");\n"
        else:
            f = ""

        if packet.gen_stats:
            s = f"    stats_{packet.name}_counters[index]++;\n"
        else:
            s = ""

        return f"""  if (fields[index++]) {{
{f}{s}    {self.get_put(True)}
  }}
"""

    def get_put(self, deltafragment):
        """
        Returns code which put this field.
        """

        # Do we need an extra arg for dio_put?
        dio_arg = ""
        if self.struct_type == "float":
            dio_arg = f", {self.float_factor}, {self.dataio_type}{{}}"
        elif "std::" in self.dataio_type:
            dio_arg = f", {self.dataio_type}{{}}"
        elif self.dataio_type == "memory":
            dio_arg = f", {self.array_size_u}"

        c = f"dio_put(dout, packet->{self.name}{dio_arg});"

        # We're done for scalar types
        if self.dataio_type == "bitvector":
            return c
        elif self.dataio_type in {"memory", "string"} and self.array_dims != 2:
            return c
        elif not self.array_dims:
            return c

        if self.dataio_type in ["string", "memory"]:
            # We handle the size via array dimensions below.
            dio_arg = ""

        if deltafragment and self.diff:
            size_args = f", old->{self.name}"
        elif self.array_dims == 2:
            # Invert sizes for recursive call
            size_args = f", {self.array_size1_u}, {self.array_size2_u}"
        else:
            size_args = f", {self.array_size_u}"

        return f"dio_put(dout, real_packet->{self.name}{size_args}{dio_arg});"

    def get_get_wrapper(self, packet):
        """
        Returns a code fragment which will get the field if the "fields"
        bitvector says so.
        """

        get = self.get_get(True)
        if self.struct_type == "bool" and not self.array_dims:
            return f"real_packet->{self.name} = fields[index++];\n"
        get = indent(get, "  ")
        if packet.gen_log:
            f = f"  {packet.log_macro}(\"  got field '{self.name}'\");\n"
        else:
            f = ""
        return f"""if (fields[index++]) {{
{f}{get}
}}
"""

    def get_get(self, deltafragment):
        """
        Returns code which get this field.
        """

        # Do we need an extra arg for dio_get?
        dio_arg = ""
        if self.struct_type == "float":
            dio_arg = f", {self.float_factor}, {self.dataio_type}{{}}"
        elif "std::" in self.dataio_type:
            dio_arg = f", {self.dataio_type}{{}}"
        elif self.dataio_type in ["string", "memory"]:
            dio_arg = f", sizeof(real_packet->{self.name})"

        # dio_get call and error checking
        c = dedent(
            f"""\
            if (!dio_get(din, real_packet->{self.name}{dio_arg})) {{
              RECEIVE_PACKET_FIELD_ERROR({self.name});
            }}"""
        )

        # We're done for scalar types
        if self.dataio_type == "bitvector":
            return c
        elif self.dataio_type == "string" and self.array_dims != 2:
            return c
        elif not self.array_dims:
            return c

        if self.dataio_type in ["string", "memory"]:
            # We handle the size via array dimensions below.
            dio_arg = ""

        if deltafragment and self.diff:
            # Array-diff
            size_args = ", array_diff{}"
        elif self.array_dims == 2:
            # Invert sizes for recursive call
            size_args = f", {self.array_size1_u}, {self.array_size2_u}"
        else:
            size_args = f", {self.array_size_u}"

        return dedent(
            f"""\
            if (!dio_get(din, real_packet->{self.name}{size_args}{dio_arg})) {{
              RECEIVE_PACKET_FIELD_ERROR({self.name});
            }}"""
        )


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

        self.cancel = []
        remaining = []
        for i in flags:
            match = re.search(r"^cancel\((.*)\)$", i)
            if match:
                self.cancel.append(match.group(1))
                continue
            remaining.append(i)
        flags = remaining

        self.capability = None
        remaining = []
        for i in flags:
            match = re.search(r"^cap\((.*)\)$", i)
            if match:
                self.capability = match.group(1)
                break
            remaining.append(i)
        flags = remaining

        assert len(flags) == 0, repr(flags)

        self.fields = []
        for i in lines:
            self.fields = self.fields + parse_fields(i, aliases)

        key_fields = list(filter(lambda x: x.is_key, self.fields))
        if not key_fields:
            self.key_field = None
        elif len(key_fields) == 1:
            self.key_field = key_fields[0]
        else:
            raise ValueError(
                f"{self.name}: At most one field per packet can have the 'key' flag"
            )

        self.other_fields = list(filter(lambda x: not x.is_key, self.fields))
        self.bits = len(self.other_fields)

        self.want_dsend = self.dsend_given

        if len(self.fields) == 0:
            self.delta = 0
            self.no_packet = 1
            assert not self.want_dsend, "dsend for a packet without fields isn't useful"

        if len(self.fields) > 5 or self.name.split("_")[1] == "ruleset":
            self.handle_via_packet = 1

        self.dsend_args = ", ".join(
            map(lambda x: x.get_handle_type() + x.name, self.fields)
        )
        if self.dsend_args:
            self.dsend_args += ","

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
        if self.key_field is not None:
            body += f"  {self.key_field.get_declar()};\n"
        for field in self.other_fields:
            body += f"  {field.get_declar()};\n"
        return intro + body + extro

    def get_prototypes(self):
        """
        Returns a code fragment which represents the prototypes of the send and
        receive functions for the header file.
        """

        result = dedent(
            f"""\
            int send_{self.name}(connection *pc,
                                 const {self.name} *packet=nullptr,
                                 bool force_to_send=false);
            """
        )
        if self.want_lsend:
            result += dedent(
                f"""\
                void lsend_{self.name}(conn_list *dest,
                                       const {self.name} *packet=nullptr,
                                       bool force_to_send=false);
                """
            )
        if self.want_dsend:
            result += dedent(
                f"""\
                int dsend_{self.name}(connection *pc,
                                      {self.dsend_args}
                                      bool force_to_send=false);
                """
            )
            if self.want_lsend:
                result += dedent(
                    f"""\
                    void dlsend_{self.name}(conn_list *dest,
                                            {self.dsend_args}
                                            bool force_to_send=false);
                    """
                )

        return result.strip() + "\n"

    def get_send(self):
        check = "true" if self.capability is None else "false"

        return dedent(
            f"""\
            int send_{self.name}(connection *pc, const {self.name} *packet, bool force_to_send)
            {{
              if (!pc->used) {{
                  qCritical("WARNING: trying to send data to the closed connection %s",
                          conn_description(pc));
                  return -1;
              }}
              if constexpr ({check}) {{
                fc_assert_ret_val_msg(pc->phs.handlers[{self.type}] != nullptr, -1,
                                      "Handler for {self.type} not installed");
              }} else if (!pc->phs.handlers[{self.type}]) {{
                return 0;
              }}
              return pc->phs.handlers[{self.type}]->send(pc, packet, force_to_send);
            }}

            """
        )

    def get_class(self):
        base_class = "packet_handler"
        if self.delta:
            if self.key_field is None:
                base_class = f"packet_delta_handler<{self.name}>"
            else:
                base_class = f"packet_delta_key_handler<{self.name}>"

        result = dedent(
            f"""\
            class {self.name}_handler : public {base_class} {{
            """
        )

        if self.delta:
            result += indent(self.get_field_count(), "  ")
            result += dedent(
                f"""\
                public:
                  {self.name}_handler(packet_capabilities_type capability) :
                      {base_class}(field_count(capability))
                  {{}}
                """
            )
        else:
            result += dedent(
                f"""\
                public:
                  {self.name}_handler(packet_capabilities_type capability)
                  {{
                    Q_UNUSED(capability);
                  }}
                """
            )
        result += f"  virtual ~{self.name}_handler() override = default;\n"
        result += indent(self.get_receive(), "  ")
        result += indent(self.get_send_member(), "  ")
        result += "};\n\n"
        return result

    def get_lsend(self):
        """
        Returns a code fragment which is the implementation of the lsend
        function.
        """

        if not self.want_lsend:
            return ""
        return dedent(
            f"""\
            void lsend_{self.name}(conn_list *dest, const {self.name} *packet,
                                  bool force_to_send)
            {{
              conn_list_iterate(dest, pconn) {{
                send_{self.name}(pconn, packet, force_to_send);
              }} conn_list_iterate_end;
            }}

            """
        )

    def get_dsend(self):
        """
        Returns a code fragment which is the implementation of the dsend
        function.
        """

        if not self.want_dsend:
            return ""
        fill = "\n        ".join(map(lambda x: x.get_fill(), self.fields))
        return dedent(
            f"""\
        int dsend_{self.name}(connection *pc,
                              {self.dsend_args}
                              bool force_to_send)
        {{
          {self.name} packet, *real_packet = &packet;

        {fill}

          return send_{self.name}(pc, real_packet);
        }}

        """
        )

    def get_dlsend(self):
        """
        Returns a code fragment which is the implementation of the dlsend
        function.
        """

        if not (self.want_lsend and self.want_dsend):
            return ""
        fill = "\n            ".join(map(lambda x: x.get_fill(), self.fields))
        return dedent(
            f"""\
            void dlsend_{self.name}(conn_list *dest,
                                   {self.dsend_args}
                                   bool force_to_send)
            {{
              {self.name} packet, *real_packet = &packet;

            {fill}

              lsend_{self.name}(dest, real_packet);
            }}

            """
        )

    def get_field_count(self):
        """
        Returns code for a function counting how many fields this packet will
        use.
        """

        code = dedent(
            f"""\
            static int field_count(packet_capabilities_type capability)
            {{
            """
        )

        base_count = len(
            list(filter(lambda field: field.condition is None, self.other_fields))
        )
        code += f"  auto fields = {base_count};\n"

        for field in self.other_fields:
            if field.condition is not None:
                code += indent(
                    dedent(
                        f"""\
                        if ({field.condition}) {{
                          fields++;
                        }}
                        """
                    ),
                    "  ",
                )

        code += dedent(
            """\
              return fields;
            }
            """
        )
        return code

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

    def get_send_member(self):
        """
        Returns a code fragment which is the implementation of the send
        function. This is one of the two real functions. So it is rather
        complex to create.
        """

        if self.gen_stats:
            report = f"""
  stats_total_sent++;
  stats_{self.name}_sent++;
"""
        else:
            report = ""
        if self.gen_log:
            if self.key_field is None:
                log = f'\n  {self.log_macro}("{self.name}: sending info");\n'
            else:
                log = f'\n  {self.log_macro}("{self.name}: sending info about ({self.key_field.name}=%d)", real_packet->{self.key_field.name});\n'
        else:
            log = ""
        if self.want_pre_send:
            pre1 = f"""
  {{
    auto tmp = new {self.name}();

    *tmp = *packet;
    pre_send_{self.name}(pc, tmp);
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
            real_packet1 = f"  const {self.name} *real_packet = packet;\n"
        else:
            real_packet1 = ""

        if not self.no_packet:
            if self.delta:
                delta_header = indent(
                    dedent(
                        f"""
                        bool differ;
                        int different = force_to_send;
                        """
                    ),
                    "  ",
                )
                body = self.get_delta_send_body(pre2)
            else:
                delta_header = ""
                body = ""
                for field in self.fields:
                    if field.condition is None:
                        body += field.get_put(False) + "\n"
                    else:
                        body += f"if ({field.condition}) {{\n"
                        body += indent(field.get_put(False), "  ") + "\n"
                        body += "}\n"
            body = body + "\n"
        else:
            body = ""
            delta_header = ""

        code = dedent(
            f"""\
            virtual int send(connection *pc, const void *packet_data,
                             bool force_to_send) override
            {{
              [[maybe_unused]]
              auto packet = static_cast<const {self.name} *>(packet_data);
            """
        )
        code += real_packet1
        code += delta_header
        code += "  QByteArray dout;\n"
        code += "  [[maybe_unused]] auto capability = pc->functional_caps;\n"
        code += log
        code += report
        code += pre1
        code += body

        # Cancel some is-info packets.
        for i in self.cancel:
            field = self.key_field or self.other_fields[0]
            code += f"\n  pc->phs.handlers[{i}]->reset(real_packet->{field.name});\n"

        code += pre2
        code += dedent(
            f"""\
              return send_packet(pc, {self.type}, dout);
            }}
            """
        )
        return code

    def get_delta_send_body(self, pre2):
        """
        Helper for get_send()
        """

        intro = dedent(
            f"""
            fields.fill(false);
            """
        )
        if self.key_field is None:
            intro += dedent(
                f"""
                if (!last_sent.has_value()) {{
                  last_sent.emplace();
                  different = 1; // Force to send
                }}
                auto old = &*last_sent;
                """
            )
        else:
            intro += dedent(
                f"""
                auto [it, created] = send_map.try_emplace(real_packet->{self.key_field.name});
                if (created) {{
                  different = 1; // Force to send
                }}
                auto old = &it->second;
                """
            )

        intro += dedent(
            """
            int index = 0; // Field index
            """
        )
        intro = indent(intro, "  ")

        body = ""
        for field in self.other_fields:
            if field.condition is None:
                body += field.get_cmp_wrapper()
            else:
                body += f"  if ({field.condition}) {{\n"
                body += indent(field.get_cmp_wrapper(), "  ")
                body += "  }\n"
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
{fl}{s}{pre2}    return 0;
  }}
"""

        body += """
  dio_put(dout, fields);
  index = 0; // Reset field index
"""

        if self.key_field is not None:
            body += self.key_field.get_put(True) + "\n"
        body += "\n"

        for field in self.other_fields:
            if field.condition is None:
                body += field.get_put_wrapper(self)
            else:
                body += f"  if ({field.condition}) {{\n"
                body += indent(field.get_put_wrapper(self), "  ")
                body += "  }\n"
        body += """
  *old = *real_packet;
"""

        return intro + body

    def get_receive(self):
        """
        Returns a code fragment which is the implementation of the receive
        function. This is one of the two real functions. So it is rather complex
        to create.
        """

        body1 = ""

        if self.delta:
            delta_body1 = """
  dio_get(din, fields);
  """
            if self.key_field is not None:
                body1 += indent(self.key_field.get_get(1), "  ") + "\n"
            body2 = self.get_delta_receive_body()
        else:
            delta_body1 = ""
            for field in self.fields:
                if field.condition is None:
                    body1 += indent(field.get_get(self.delta), "  ") + "\n"
                else:
                    body1 += f"  if ({field.condition}) {{\n"
                    body1 += indent(field.get_get(self.delta), "    ") + "\n"
                    body1 += "  }\n"

            body2 = ""

        body1 += "\n"

        if self.gen_log:
            if self.key_field is None:
                log = f'  {self.log_macro}("{self.name}: got info");\n'
            else:
                log = f'  {self.log_macro}("{self.name}: got info about ({self.key_field.name}=%d)", real_packet->{self.key_field.name});\n'
        else:
            log = ""

        code = dedent(
            f"""\
            virtual void *receive(connection *pc) override
            {{
              RECEIVE_PACKET_START({self.name}, real_packet);
              [[maybe_unused]] auto capability = pc->functional_caps;
            """
        )
        code += delta_body1
        code += body1
        code += log
        code += body2

        # Cancel some is-info packets.
        for i in self.cancel:
            field = self.key_field or self.other_fields[0]
            code += f"\n  pc->phs.handlers[{i}]->reset(real_packet->{field.name});\n"

        code += dedent(
            f"""\
              RECEIVE_PACKET_END(real_packet);
            }}
            """
        )
        return code

    def get_delta_receive_body(self):
        """
        Helper for get_receive()
        """

        # At this stage real_packet is default-constructed with only the key, so
        # try_emplace() will return the old one or the default-constructed one.
        # In either case the key is already correct.
        if self.key_field is None:
            body = dedent(
                f"""
                if (!last_received.has_value()) {{
                  last_received = *real_packet;
                }}
                real_packet = &*last_received;
                """
            )
        else:
            body = dedent(
                f"""
                auto [it, _] = receive_map.try_emplace(real_packet->{self.key_field.name}, *real_packet);
                real_packet = &it->second;
                """
            )

        # Field index
        body += f"\nint index = 0;\n"

        for field in self.other_fields:
            if field.condition is None:
                body += field.get_get_wrapper(self)
            else:
                body += f"if ({field.condition}) {{\n"
                body += indent(field.get_get_wrapper(self), "  ")
                body += "}\n"

        return indent(body, "  ")


def packet_capability_name(cap: str) -> str:
    """
    Turns a network capability name of the form cap-name into an enumerator of
    the form PC_CAP_NAME.
    """

    return "PC_" + cap.upper().replace("-", "_")


def get_all_capabilities(packets: list[Packet]) -> list[str]:
    """
    Extracts the list of all capabilities needed by the packets.
    """

    all_caps = set()
    for p in packets:
        if p.capability is not None:
            all_caps.add(p.capability)

        for f in p.fields:
            if f.capability is not None:
                all_caps.add(f.capability)

    return sorted(all_caps)


def get_capability_specenum(packets: list[Packet]) -> str:
    """
    Returns a code fragment defining the packet_capability specenum.
    """

    code = "#define SPECENUM_NAME packet_capability\n"
    for i, cap in enumerate(get_all_capabilities(packets)):
        code += f"#define SPECENUM_VALUE{i} {packet_capability_name(cap)}\n"
        code += f'#define SPECENUM_VALUE{i}NAME "{cap}"\n'
    code += dedent(
        """\
        #define SPECENUM_COUNT PC_COUNT
        #include "specenum_gen.h"
        """
    )
    return code


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
        for _ in range(last + 1, n):
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
        for _ in range(last + 1, n):
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

    intro = dedent(
        """\
        void packet_handlers_fill_initial(packet_handlers &handlers,
                                          packet_capabilities_type capability)
        {
        """
    )

    for cap in get_all_capabilities(packets):
        intro += f"""  fc_assert_msg(has_capability("{cap}", our_capability),
                "Packets have support for unknown '{cap}' capability!");
"""

    body = ""
    for p in packets:
        body += (
            f"  handlers[{p.type}] = std::make_unique<{p.name}_handler>(capability);\n"
        )

    extro = """
}

"""
    return intro + body + extro


def get_packet_handlers_fill_capability(packets: list[Packet]) -> str:
    """
    Returns a code fragment which is the implementation of the
    packet_handlers_fill_capability() function.
    """

    intro = dedent(
        """\
        void packet_handlers_fill_capability(packet_handlers &handlers,
                                             packet_capabilities_type capability)
        {
          packet_handlers_fill_initial(handlers, capability);
        """
    )

    body = ""
    for p in packets:
        body += (
            f"handlers[{p.type}] = std::make_unique<{p.name}_handler>(capability);\n"
        )

    # Packets controlled by capabilities
    for packet in packets:
        if packet.capability is None:
            continue

        cap = packet_capability_name(packet.capability)
        body += dedent(
            f"""
            if (!(capability & (1 << {cap}))) {{
              log_packet_detailed("{packet.type}: will not send, cap=0x%x",
                                  capability);
              handlers[{packet.type}] = nullptr;
            }}
            """
        )

    extro = "}\n"
    return intro + indent(body, "  ") + extro


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

// utility
#include "shared.h" // MAX_LEN_ADDR

// common
#include "cm.h"
#include "fc_types.h"
#include "unit.h"

// std
#include <array>

"""
    )

    # Write capability enum
    output.write(get_capability_specenum(packets))

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
// self - generated
#include <packets_gen.h>

// utility
#include "capability.h"
#include "log.h"
#include "support.h"

// common
#include "actions.h"
#include "capstr.h"
#include "connection.h"
#include "dataio_raw.h"
#include "fc_types.h"
#include "packets.h"
#include "protocol.h"
#include "requirements.h"
#include "unit.h"
#include "worklist.h"

// Qt
#include <QBitArray>
#include <QtLogging> // qDebug, qWarning, qCritical, etc

// std
#include <cstdlib> // EXIT_FAILURE, free, at_quick_exit
#include <cstring> // str*, mem*
"""
    )
    output.write(
        dedent(
            """
        static_assert(PC_COUNT <= sizeof(packet_capabilities_type),
                      "Resize packet_capabilities_type");
        """
        )
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

    # write send and receive
    for packet in packets:
        output.write(packet.get_class())
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
            def format_field(field):
                if field.array_dims > 0 and not field.dataio_type == "string":
                    return f"{packet_cast}->{field.name}.data()"
                return f"{packet_cast}->{field.name}"

            args = ",\n      ".join(map(format_field, packet.fields))
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

struct server_connection;

bool server_handle_packet(enum packet_type type, const void *packet,
                          player *pplayer, server_connection *pconn);

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
                    f"void {handle_fn_name}(server_connection *pc, const {packet.name} *packet);\n"
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
                output.write(f"void {handle_fn_name}(server_connection *pc{args});\n")
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
                          player *pplayer, server_connection *pconn)
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
                if field.array_dims > 0 and not field.dataio_type == "string":
                    arg += ".data()"
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
