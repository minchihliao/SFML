project "SFML"
    kind "StaticLib"
    language "C++"

    targetdir ("bin/" .. Outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. Outputdir .. "/%{prj.name}")

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
        "src",
        "extlibs/headers" -- 假設所有第三方頭文件都放在這個資料夾下
    }

    libdirs {
        "extlibs/bin/x64" -- 假設你的構建目標是64位系統
    }

    defines {
        "SFML_STATIC"
    }

    filter "system:windows"
        links {
            "openal32" -- 引用 extlibs 中的 OpenAL 庫
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"