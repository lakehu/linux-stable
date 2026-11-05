# gdb settings for this project, read automatically when gdb is started in
# this directory. gdb only reads it if the directory is allowed, e.g. with
#   set auto-load safe-path /        or
#   add-auto-load-safe-path <this directory>
# in ~/.gdbinit. VS Code (.vscode/launch.json) starts gdb with -nx and does
# not use it.
#
# With gdb-dashboard installed (https://github.com/cyrus-and/gdb-dashboard),
# use its compact x86 layout from x86-qemu.gdb; without it, plain gdb.
python
try:
    gdb.execute('help dashboard', to_string=True)
except gdb.error:
    pass
else:
    gdb.execute('source x86-qemu.gdb')
end
