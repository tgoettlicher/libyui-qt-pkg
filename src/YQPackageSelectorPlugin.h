/**************************************************************************
Copyright (C) 2000 - 2010 Novell, Inc.
All Rights Reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

**************************************************************************/



/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|						     (c) SuSE Linux AG |
\----------------------------------------------------------------------/

  File:	      YQPackageSelectorPlugin.h

  Author:     Stefan Hundhammer <sh@suse.de>

/-*/


#ifndef YQPackageSelectorPlugin_h
#define YQPackageSelectorPlugin_h

#include <YPackageSelectorPlugin.h>
#include "pkg/YQPackageSelector.h"
#include "pkg/YQPatternSelector.h"
#include "pkg/YQSimplePatchSelector.h"


/**
 * Simplified access to the Qt UI's package selector plugin.
 **/
class YQPackageSelectorPlugin: public YPackageSelectorPlugin
{
public:

    /**
     * Constructor: Load the plugin library for the Qt package selector.
     **/
    YQPackageSelectorPlugin();

    /**
     * Destructor. Calls dlclose() which will unload the plugin library if it
     * is no longer used, i.e. if the reference count dlopen() uses reaches 0.
     **/
    virtual ~YQPackageSelectorPlugin();

    /**
     * Create a package selector.
     * Implemented from YPackageSelectorPlugin.
     *
     * This might return 0 if the plugin lib could not be loaded or if the
     * appropriate symbol could not be located in the plugin lib. 
     **/
    virtual YQPackageSelector * createPackageSelector	( YWidget * 	parent,
							  long		modeFlags = 0 );
    
    /**
     * Create a pattern selector.
     *
     * This might return 0 if the plugin lib could not be loaded or if the
     * appropriate symbol could not be located in the plugin lib. 
     **/
    virtual YQPatternSelector * createPatternSelector	( YWidget * 	parent,
							  long		modeFlags = 0 );
    /**
     * Create a simple patch selector.
     *
     * This might return 0 if the plugin lib could not be loaded or if the
     * appropriate symbol could not be located in the plugin lib. 
     **/
    virtual YQSimplePatchSelector * createSimplePatchSelector( YWidget * parent,
							       long	modeFlags = 0 );
};



#endif // YQPackageSelectorPlugin_h
