include "Common.lua"

-- 4073: init_seg(lib)
disablewarnings "4073"

project "Luft"
	kind "StaticLib"

	SetConfigurationSettings()
	UseWindowsSettings()

	includedirs { "Source" }
	files {
		"Source/Luft/**.cpp", "Source/Luft/**.hpp",
		"Luft.natvis"
	}

	filter {}
