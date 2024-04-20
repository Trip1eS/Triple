add_rules("mode.debug", "mode.release")
set_languages("cxxlatest")

triple_dir = os.scriptdir()

add_requires("catch2", "glfw", "stb")
add_packages("stb")

includes("thirdparty")
includes("modules")
includes("samples/001-helloworld")