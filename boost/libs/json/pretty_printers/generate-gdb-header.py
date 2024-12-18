#!/usr/bin/env python
#
# Copyright (c) 2024 Dmitry Arkhipov (grisumbras@yandex.ru)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/boostorg/json
#

import argparse
import os
import random
import re
import sys


_top = '''\
#if defined(__ELF__)

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Woverlength-strings"
#elif defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Woverlength-strings"
#endif

__asm__(
  ".pushsection \\\".debug_gdb_scripts\\\", \\\"MS\\\",@progbits,1\\n"
  ".ascii \\\"\\\\4gdb.inlined-script.{script_id}\\\\n\\\"\\n"
'''

_bottom = '''\
  ".byte 0\\n"
  ".popsection\\n");
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

#endif // defined(__ELF__)
'''


class Nullcontext():
    def __init__(self):
        pass

    def __enter__(self):
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        pass


def parse_args(args):
    parser = argparse.ArgumentParser(
        prog=args[0],
        description=(
            'Converts a Python script into a C header '
            'that pushes that script into .debug_gdb_scripts ELF section'))
    parser.add_argument(
        'input',
        help='Input file')
    parser.add_argument(
        'output',
        nargs='?',
        help='Output file; STDOUT by default')
    parser.add_argument(
        '--header-guard',
        help=(
            'Header guard macro to use; '
            'by default deduced from the output file name'))
    parser.add_argument(
        '--disable-macro',
        default='BOOST_ALL_NO_EMBEDDED_GDB_SCRIPTS',
        help=(
            'Macro to disable pretty printer embedding; '
            'by default BOOST_ALL_NO_EMBEDDED_GDB_SCRIPTS'))
    return parser.parse_args(args[1:])

def write_front_matter(header, header_guard, disable_macro, source):
    header.write(
        '// Autogenerated from %s by boost-pretty-printers\n\n' % os.path.basename(source))
    if header_guard:
        header.write('#ifndef %s\n' % header_guard)
        header.write('#define %s\n\n' % header_guard)
    if disable_macro:
        header.write('#ifndef %s\n\n' % disable_macro)
    header.write(
        _top.format(script_id=header_guard or str(random.random()) ))

def write_back_matter(header, header_guard, disable_macro):
    header.write(_bottom)
    if disable_macro:
        header.write('\n#endif // %s\n' % disable_macro)
    if header_guard:
        header.write('\n#endif // %s\n' % header_guard)

def main(args, stdin, stdout):
    args = parse_args(args)

    header_guard = args.header_guard
    if header_guard is None:
        if args.output:
            header_guard = os.path.basename(args.output).upper()

    if args.output:
        header = open(args.output, 'w', encoding='utf-8')
        header_ctx = header
    else:
        header = stdout
        header_ctx = Nullcontext()

    not_whitespace = re.compile('\\S', re.U)

    with open(args.input, 'r', encoding='utf-8') as script:
        with header_ctx:
            check_for_attribution = True
            for line in script:
                if not not_whitespace.search(line):
                    header.write('\n')
                    continue

                if check_for_attribution:
                    if line.startswith('#!'): # shebang
                        continue
                    elif line.startswith('#'):
                        header.write('// ')
                        header.write(line[1:])
                        continue
                    else:
                        write_front_matter(
                            header,
                            header_guard,
                            args.disable_macro,
                            args.input)
                        check_for_attribution = False

                header.write('  ".ascii \\"')
                header.write(line[:-1])
                header.write('\\\\n\\"\\n"\n')

            write_back_matter(header, header_guard, args.disable_macro)


if __name__ == '__main__':
    main(sys.argv, sys.stdin, sys.stdout)
