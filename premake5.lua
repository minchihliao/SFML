project "SFML"
    kind "StaticLib"
    language "C++"

    targetdir ("bin/" .. Outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. Outputdir .. "/%{prj.name}")


    -- 全平台通用配置
    includedirs 
    { 
        "include"
    }

    files
	{
		"include/SFML/**.hpp",
		"include/Audio/**.hpp",
		"include/Network/**.hpp",
		"include/Graphics/**.hpp",
		"include/System/**.hpp",
		"include/Window/**.hpp",
		"src/SFML/Audio/**.cpp",
		"src/SFML/Graphics/**.cpp",
	}

    -- 通用預處理器定義
    defines
    {
        "SFML_STATIC"
    }


    filter { "system:windows", "configurations:Debug" }
        buildoptions "/MTD"
        systemversion "latest"
        runtime "Debug"
        symbols "on"
        files
        {
            "src/SFML/Network/Win32/**.cpp",
            "src/SFML/Network/Win32/**.hpp",
            "src/SFML/Network/System/**.cpp",
            "src/SFML/Network/System/**.hpp",
            "src/SFML/Network/Window/**.cpp",
            "src/SFML/Network/Window/**.hpp",
        }
        defines
        {
            "WIN32",
            "_WINDOWS",
            "DEBUG",
            "SFML_STATIC"
        }
        
    -- Windows 平台設定
    filter { "system:windows", "configurations:Release" }
        buildoptions "/MT"
        systemversion "latest"
        runtime "Release"
        optimize "on"
        files
        {
            "src/SFML/Network/Win32/**.cpp",
            "src/SFML/Network/Win32/**.hpp",
            "src/SFML/Network/System/**.cpp",
            "src/SFML/Network/System/**.hpp",
            "src/SFML/Network/Window/**.cpp",
            "src/SFML/Network/Window/**.hpp",
        }
        defines
        {
            "WIN32",
            "_WINDOWS",
            "SFML_STATIC"
        }
        
     

        