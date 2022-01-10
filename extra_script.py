Import("env")

# Add semihosting feature
env.Append(
    LINKFLAGS=["--specs=rdimon.specs"],
    LIBS=["rdimon"]
)

# set path for compile commands
env.Replace(COMPILATIONDB_PATH=os.path.join("$PROJECT_DIR", "compile_commands.json"))