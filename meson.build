project('gst-plugins-mimo', 'c',
  version : '1.22.0',
  default_options : [ 'warning_level=1',
                      'buildtype=debugoptimized' ])

cdata = configuration_data()
cdata.set_quoted('PACKAGE', meson.project_name())
configure_file(output : 'config.h', configuration : cdata)

library('gstmimo',
	['plugin.c', 'gstmimofilter.h', 'gstmimofilter.c', 'api.h', 'api.c'],
	dependencies: [
		dependency('gstreamer-1.0'),
		dependency('gstreamer-video-1.0'),
	],
	install_dir: join_paths(get_option('libdir'), 'gstreamer-1.0')
)
