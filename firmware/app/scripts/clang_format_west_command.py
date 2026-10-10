#!/usr/bin/env python

# Copyright (c) 2026 Daniel Kampert
# SPDX-License-Identifier: Apache-2.0

import sys
import shutil
import subprocess

from pathlib import Path
from west.commands import WestCommand

class FormatCodeWestCommand(WestCommand):
    def __init__(self):
        super().__init__(
            "format",
            "Format the codebase",
            """Use to format all .c and .h files with clang-format""",
        )

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name, help = self.help, description = self.description
        )

        parser.add_argument(
            "--dry-run",
            action = "store_true",
            help = "Dry run won't actually format the code.",
        )

        return parser

    def do_run(self, args, _unknown_args):
        if(shutil.which("clang-format") is None):
            print("ERROR: 'clang-format' not found in PATH", file = sys.stderr)
            sys.exit(127)

        # The style is taken from the .clang-format file in the app directory
        app = Path(__file__).parent.parent
        files = sorted(str(file) for file in (app / "src").rglob("*") if file.suffix in (".c", ".h"))
        if(len(files) == 0):
            print("No source files found")
            sys.exit(0)

        clang_format_args = ["clang-format", "--style=file"]
        if(args.dry_run):
            clang_format_args.extend(["--dry-run", "--Werror"])
        else:
            clang_format_args.append("-i")
        clang_format_args.extend(files)

        sys.exit(subprocess.run(clang_format_args).returncode)
