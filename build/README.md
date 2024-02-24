
These two files control repo-wide build settings.

Msbuild projects work by including a "default" props file, then allowing
projects to override those defaults, then including a "props" file that
can apply high-level properties into lower level configuration.  After that,
projects can once again override lower level values, then define which files
to compile, then include a "targets" file to perform execution.

In other words, the way to think of these two files is
`Winfile.default.props` executes before `Winfile.props`, and the two files
are split to allow individual projects to manipulate state in between.

Generally, anything affecting supported architectures, compilers, and basic
compilation options should be here.  Anything specific to an executable
should be in the project files.

