project "SFML"
    kind "StaticLib"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        "src/SFML/Graphics/**.cpp",
        "src/SFML/Window/**.cpp",
        "src/SFML/System/**.cpp",
        "src/SFML/Audio/**.cpp",
        "src/SFML/Network/**.cpp",
        "include/**.hpp",
        "include/**.inl"
    }
    
    includedirs {
        "include",
        "src"
    }

    defines {
        "SFML_STATIC"
    }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"