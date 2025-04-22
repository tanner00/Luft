include "Common.lua"

project "Luft"
	kind "StaticLib"

	SetConfigurationSettings()
	UseWindowsSettings("")

	includedirs { "Source" }
	files { "Source/Luft/**.cpp", "Source/Luft/**.hpp", "Luft.natvis" }

	filter {}
