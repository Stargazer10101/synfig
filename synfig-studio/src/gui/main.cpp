/* === S Y N F I G ========================================================= */
/*!	\file gui/main.cpp
**	\brief Synfig Studio Entrypoint
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2007, 2008 Chris Moore
**
**	This file is part of Synfig.
**
**	Synfig is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 2 of the License, or
**	(at your option) any later version.
**
**	Synfig is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with Synfig.  If not, see <https://www.gnu.org/licenses/>.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <glibmm/convert.h>
#include <glibmm/miscutils.h>

#include <synfig/os.h>

#include <gui/app.h>
#include <gui/exception_guard.h>
#include <gui/localization.h>

#include <iostream>

// macOS-specific headers
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <stdlib.h>
#include <sys/param.h> // For PATH_MAX
#endif

#endif

/* === U S I N G =========================================================== */

using namespace synfig;
using namespace studio;

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

/* === P R O C E D U R E S ================================================= */

#ifdef __APPLE__
void setupMacOSEnvironment() {
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (!mainBundle) {
        return; // Not running in a bundle, do nothing.
    }

    CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
    if (!resourcesURL) {
        return;
    }

    char resourcesPath[PATH_MAX];
    if (!CFURLGetFileSystemRepresentation(resourcesURL, true, (UInt8 *)resourcesPath, PATH_MAX)) {
        CFRelease(resourcesURL);
        return;
    }
    CFRelease(resourcesURL);

    std::string resourcesPathStr(resourcesPath);
    std::string contentsPathStr = resourcesPathStr.substr(0, resourcesPathStr.rfind('/'));
    
    // Define paths based on the CORRECTED bundle layout
    std::string frameworksPath = contentsPathStr + "/Frameworks";
    std::string sharePath = resourcesPathStr + "/share";
    std::string resourcesEtcPath = resourcesPathStr + "/etc";
    std::string modulesPath = resourcesPathStr + "/synfig/modules";
    std::string moduleListPath = resourcesEtcPath + "/synfig_modules.cfg";
    std::string pixbufLoadersPath = resourcesPathStr + "/lib/gdk-pixbuf-2.0/2.10.0/loaders";
    std::string gdkPixbufCache = resourcesPathStr + "/loaders.cache";
    std::string gsettingsSchemas = sharePath + "/glib-2.0/schemas";

    // Set Environment Variables
    // Force linker to prioritize bundled libraries to prevent duplicates
    setenv("DYLD_LIBRARY_PATH", frameworksPath.c_str(), 1);

    // Point Synfig to its bundled resources
    setenv("SYNFIG_ROOT", resourcesPathStr.c_str(), 1);
    setenv("SYNFIG_MODULE_LIST", moduleListPath.c_str(), 1);
    setenv("LTDL_LIBRARY_PATH", modulesPath.c_str(), 1);

    // Configure GTK and other libraries to find their data
    setenv("GTK_DATA_PREFIX", resourcesPathStr.c_str(), 1);
    setenv("GSETTINGS_SCHEMA_DIR", gsettingsSchemas.c_str(), 1);
    setenv("GDK_PIXBUF_MODULE_FILE", gdkPixbufCache.c_str(), 1);
    setenv("GDK_PIXBUF_MODULEDIR", pixbufLoadersPath.c_str(), 1);
    
    // Set icon theme path
    std::string iconThemePath = sharePath + "/icons";
    setenv("GTK_ICON_THEME_PATH", iconThemePath.c_str(), 1);
    
    // Prepend bundled share path to XDG_DATA_DIRS
    const char* oldXdgDataDirs = getenv("XDG_DATA_DIRS");
    std::string newXdgDataDirs = sharePath;
    if (oldXdgDataDirs) {
        newXdgDataDirs += ":";
        newXdgDataDirs += oldXdgDataDirs;
    }
    setenv("XDG_DATA_DIRS", newXdgDataDirs.c_str(), 1);
    
    // Set locale path for translations
    std::string localePath = sharePath + "/locale";
    setenv("LOCALE_PATH", localePath.c_str(), 1);
}
#endif // __APPLE__

/* === M E T H O D S ======================================================= */

/* === E N T R Y P O I N T ================================================= */

int main(int argc, char **argv)
{

	#ifdef __APPLE__
		setupMacOSEnvironment();
	#endif

	synfig::OS::fallback_binary_path = filesystem::Path(Glib::filename_to_utf8(argv[0]));
	const filesystem::Path rootpath = synfig::OS::get_binary_path().parent_path().parent_path();
	
#ifdef ENABLE_NLS
	filesystem::Path locale_dir;
	locale_dir = rootpath / filesystem::Path("share/locale");
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, locale_dir.u8_str() );
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif
	
	std::cout << std::endl;
	std::cout << "   " << _("synfig studio -- starting up application...") << std::endl << std::endl;

	SYNFIG_EXCEPTION_GUARD_BEGIN()
	
	Glib::RefPtr<studio::App> app = studio::App::instance();

	app->signal_startup().connect([app, rootpath]() {
		app->init(rootpath.u8string());
	});

	Glib::set_prgname("org.synfig.SynfigStudio");

	app->register_application();
	if (app->is_remote()) {
		std::cout << std::endl;
		std::cout << "   " << _("synfig studio is already running") << std::endl << std::endl;
		std::cout << "   " << _("the existing process will be used") << std::endl << std::endl;
	}

	int exit_code = app->run(argc, argv);

	std::cerr << "Application appears to have terminated successfully" << std::endl;

	return exit_code;

	SYNFIG_EXCEPTION_GUARD_END_INT(0)
}
