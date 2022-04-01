{
	'targets': [
		{
			'target_name': 'pcre2',
			'sources': [ 'src/addon.cc', 'src/PCRE2Wrapper.cc' ],
			'actions': [
				{
					'action_name': 'build PCRE2 library',
					'message': 'Building PCRE2 library...',
					'inputs': [''],
					'outputs': [''],
					'action': ['/bin/sh', './compile'],
				},
			],
			"include_dirs": [
				"<!(node -e \"require('nan')\")",
				"build_pcre2/include/",
				"deps/jpcre2/src/"
			],
			"libraries": [ "../build_pcre2/lib/libpcre2-8.a" ],
			'conditions': [
				[ 'OS!="win"', {
					'cflags_cc+': [ '-std=c++17' ],
				}],
				[ 'OS=="win"', {
					'cflags_cc+': [ '/std:c++17' ],
				}],
			],
			'defines': [ 'JPCRE2_UNSET_CAPTURES_NULL' ],
		},
	]
}
