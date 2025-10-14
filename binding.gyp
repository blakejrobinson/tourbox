{
	"targets": 
	[
		{
			"target_name": "tourbox",
			"cflags!": [ "-fno-exceptions" ],
			"cflags_cc!": [ "-fno-exceptions" ],
			"sources": 
			[
				"src/tourbox_addon.cc",
				"src/tourbox_server.cc",
				"src/tourbox_client.cc"
			],
			"include_dirs": [
				"node_modules/node-addon-api",
				"src"
			],
			"defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
			"conditions": [
				["OS=='win'", {
					"libraries": [
						"-lws2_32.lib",
						"-luser32.lib",
						"-lgdi32.lib"
					],
					"msvs_settings": {
						"VCCLCompilerTool": {
							"ExceptionHandling": 1
						}
					}
				}
				],
				["OS=='mac'", {
					"xcode_settings": {
						"MACOSX_DEPLOYMENT_TARGET": "10.13",
						"CLANG_CXX_LIBRARY": "libc++",
						"GCC_ENABLE_CPP_EXCEPTIONS": "YES"
					}
				}
				]
			]
		}
	]
}