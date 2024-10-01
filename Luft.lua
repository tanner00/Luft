include "Common.lua"

project "Luft"
	kind "StaticLib"

	SetConfigurationSettings()
	UseWindowsSettings()

	includedirs { "Source" }
	files { "Source/**.cpp", "Source/**.hpp", "Luft.natvis" }

	filter {}
