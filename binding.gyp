{
  'targets': [
    {
      'target_name': 'uiohook_napi',
      'sources': [
        'libuiohook/src/logger.c',
        'src/lib/addon.c',
        'src/lib/napi_helpers.c',
        'src/lib/uiohook_worker.c',
      ],
      'include_dirs': [
        'libuiohook/include',
        'libuiohook/src',
        'src/lib',
      ],
      "conditions": [
        ['OS=="win"', {
      	  'sources': [
            'libuiohook/src/windows/input_helper.c',
            'libuiohook/src/windows/input_hook.c',
            'libuiohook/src/windows/post_event.c',
            'libuiohook/src/windows/system_properties.c'
          ],
          'include_dirs': [
            'libuiohook/src/windows'
          ]
      	}]
      ]
    }
  ]
}