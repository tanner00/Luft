include "Common.lua"

project "Erde"
	kind "StaticLib"

	SetConfigurationSettings()
	UseWindowsSettings()

	includedirs { "Source" }
	files { "Source/**.cpp", "Source/**.hpp" }

	filter {}
