def _cc_py_runtime_impl(ctx):
    toolchain = ctx.toolchains["@bazel_tools//tools/python:toolchain_type"]
    py3_runtime = toolchain.py3_runtime
    imports = []
    for dep in ctx.attr.deps:
        imports.append(dep[PyInfo].imports)
    python_path = ""
    for path in depset(transitive = imports).to_list():
        python_path += "external/" + path + ":"

    py3_runfiles = ctx.runfiles(files = py3_runtime.files.to_list())
    runfiles = [py3_runfiles]
    for dep in ctx.attr.deps:
        dep_runfiles = ctx.runfiles(files = dep[PyInfo].transitive_sources.to_list())
        runfiles.append(dep_runfiles)
        runfiles.append(dep[DefaultInfo].default_runfiles)

    runfiles = ctx.runfiles().merge_all(runfiles)

    return [
        DefaultInfo(runfiles = runfiles),
        platform_common.TemplateVariableInfo({
            "PYTHON3": str(py3_runtime.interpreter.path),
            "PYTHONPATH": python_path,
        }),
    ]

_cc_py_runtime = rule(
    attrs = {
        "deps": attr.label_list(providers = [PyInfo]),
    },
    toolchains = [
        str(Label("@bazel_tools//tools/python:toolchain_type")),
    ],
    implementation = _cc_py_runtime_impl,
)

def cc_py_test(name, py_deps = [], **kwargs):
    py_runtime_target = name + "_py_runtime"
    _cc_py_runtime(
        name = py_runtime_target,
        deps = py_deps,
    )

    kwargs.update({
        "data": kwargs.get("data", []) + [":" + py_runtime_target],
        "env": {"__PYVENV_LAUNCHER__": "$(PYTHON3)", "PYTHONPATH": "$(PYTHONPATH)"},
        "toolchains": kwargs.get("toolchains", []) + [":" + py_runtime_target],
    })

    native.cc_test(
        name = name,
        **kwargs
    )

def cc_py_binary(name, py_deps = [], **kwargs):
    py_runtime_target = name + "_py_runtime"
    _cc_py_runtime(
        name = py_runtime_target,
        deps = py_deps,
    )

    kwargs.update({
        "data": kwargs.get("data", []) + [":" + py_runtime_target],
        "env": {"__PYVENV_LAUNCHER__": "$(PYTHON3)", "PYTHONPATH": "$(PYTHONPATH)"},
        "toolchains": kwargs.get("toolchains", []) + [":" + py_runtime_target],
    })

    native.cc_binary(
        name = name,
        **kwargs
    )

def _py_embedded_libs_impl(ctx):
    deps = ctx.attr.deps
    toolchain = ctx.toolchains["@bazel_tools//tools/python:toolchain_type"]
    py3_runtime = toolchain.py3_runtime

    # addresses that need to be added to python sys.path
    all_imports = []
    for lib in deps:
        all_imports.append(lib[PyInfo].imports)
    imports_txt = "\n".join(depset(transitive = all_imports).to_list())
    imports_file = ctx.actions.declare_file(ctx.attr.name + ".imports")
    ctx.actions.write(imports_file, imports_txt)

    python_home_txt = str(py3_runtime.interpreter.dirname)
    python_home_file = ctx.actions.declare_file(ctx.attr.name + ".python_home")
    ctx.actions.write(python_home_file, python_home_txt)

    py3_runfiles = ctx.runfiles(files = py3_runtime.files.to_list())
    dep_runfiles = [py3_runfiles]
    for lib in deps:
        lib_runfiles = ctx.runfiles(files = lib[PyInfo].transitive_sources.to_list())
        dep_runfiles.append(lib_runfiles)
        dep_runfiles.append(lib[DefaultInfo].default_runfiles)

    runfiles = ctx.runfiles().merge_all(dep_runfiles)

    return [DefaultInfo(
        files = depset([imports_file, python_home_file]),
        runfiles = runfiles,
    )]

# collect paths to all files of a python library and generate .imports file and .python_home file
py_embedded_libs = rule(
    implementation = _py_embedded_libs_impl,
    attrs = {
        "deps": attr.label_list(
            providers = [PyInfo],
        ),
    },
    toolchains = [
        str(Label("@bazel_tools//tools/python:toolchain_type")),
    ],
)
