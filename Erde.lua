include "Common.lua"

project "Erde"
	kind "StaticLib"

	includedirs { "Source" }

	SetConfigurationSettings()
	UseWindowsSettings()

	files { "Source/**.cpp", "Source/**.hpp" }

	filter {}
