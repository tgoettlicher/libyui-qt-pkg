/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|							 (C) SuSE GmbH |
\----------------------------------------------------------------------/

  File:	      YQPkgLangList.cc

  Author:     Stefan Hundhammer <sh@suse.de>

  Textdomain "packages-qt"

/-*/


#define YUILogComponent "qt-pkg"
#include "YUILog.h"
#include <qregexp.h>

#include <QHeaderView>
#include "YQi18n.h"
#include "utf8.h"
#include "YQPkgLangList.h"

using std::set;


YQPkgLangList::YQPkgLangList( QWidget * parent )
    : YQPkgObjList( parent )
{
    yuiDebug() << "Creating language list" << endl;

    int numCol = 0;
    QStringList headers;
    headers <<  "";	_statusCol	= numCol++;

    // Translators: Table column heading for language ISO code like "de_DE", "en_US"
    // Please keep this short to avoid stretching the column too wide!
    headers <<  _( "Code"	);	_nameCol	= numCol++;

    // Full (human readable) language / country name like "German (Austria)"
    headers <<  _( "Language");	_summaryCol	= numCol++;
    setAllColumnsShowFocus( true );
    setHeaderLabels(headers);

    setSortingEnabled( true );
    header()->setSortIndicatorShown( true );
    header()->setClickable( true );

    sortByColumn( nameCol(), Qt::AscendingOrder );

    connect( this, 	SIGNAL( currentItemChanged        ( QTreeWidgetItem *, QTreeWidgetItem * ) ),
	     this, 	SLOT  ( filter()                                    ) );

    fillList();
    selectSomething();

    yuiDebug() << "Creating language list done" << endl;
}


YQPkgLangList::~YQPkgLangList()
{
    // NOP
}


void
YQPkgLangList::fillList()
{
    clear();
    yuiDebug() << "Filling language list" << endl;

    zypp::LocaleSet locales = zypp::getZYpp()->pool().getAvailableLocales();

    for ( zypp::LocaleSet::const_iterator it = locales.begin();
	  it != locales.end();
	  ++it )
    {
        addLangItem( *it );
    }

    yuiDebug() << "Language list filled" << endl;
}


void
YQPkgLangList::filterIfVisible()
{
    if ( isVisible() )
	filter();
}


void
YQPkgLangList::filter()
{
    emit filterStart();

    if ( selection() )
    {
        zypp::Locale lang = selection()->zyppLang();

        string currentLang = lang.name();

#warning FIXME: need a way to find out what packages provide language support
        // not yet implemented
    }

    emit filterFinished();
}


void
YQPkgLangList::addLangItem( const zypp::Locale &zyppLang )
{
    new YQPkgLangListItem( this, zyppLang );
}


YQPkgLangListItem *
YQPkgLangList::selection() const
{
    QTreeWidgetItem * item = currentItem();

    if ( ! item )
	return 0;

    return dynamic_cast<YQPkgLangListItem *> (item);
}


void
YQPkgLangList::updateActions( YQPkgObjListItem * item)
{
#if 0
    YQPkgLangListItem *litem;
    if ( !item)
        litem = dynamic_cast<YQPkgLangListItem *> ( currentItem() );
    else
        litem = dynamic_cast<YQPkgLangListItem *> ( item );
#endif

    actionSetCurrentInstall->setEnabled( true );
    actionSetCurrentDontInstall->setEnabled( true );
    actionSetCurrentTaboo->setEnabled( true );
    actionSetCurrentProtected->setEnabled( false );

    actionSetCurrentKeepInstalled->setEnabled( false );
    actionSetCurrentDelete->setEnabled( false );
    actionSetCurrentUpdate->setEnabled( false );
}


YQPkgLangListItem::YQPkgLangListItem( YQPkgLangList * 	langList,
                                      const zypp::Locale &lang )
    : YQPkgObjListItem( langList )
    , _zyppLang( lang )
{
    init();
}



YQPkgLangListItem::~YQPkgLangListItem()
{
    // NOP
}


void
YQPkgLangListItem::applyChanges()
{
    solveResolvableCollections();
}

void
YQPkgLangListItem::init()
{
    // DO NOT CALL PARENT CLASS
    _debugIsBroken	= false;
    _debugIsSatisfied	= false;
    _candidateIsNewer	= false;
    _installedIsNewer 	= false;

    if ( nameCol()    >= 0 )	setText( nameCol(),	_zyppLang.code()	);
    if ( summaryCol() >= 0 )	setText( summaryCol(),	_zyppLang.name()	);

    setStatusIcon();
}

ZyppStatus YQPkgLangListItem::status() const
{
    if ( zypp::getZYpp()->pool().isRequestedLocale( _zyppLang ) )
        return S_Install;
    else
        return S_NoInst;
}

bool YQPkgLangListItem::bySelection() const
{
    return zypp::getZYpp()->pool().isRequestedLocale( _zyppLang );
}


void YQPkgLangListItem::cycleStatus()
{
    if ( zypp::getZYpp()->pool().isRequestedLocale( _zyppLang ) )
    {
        zypp::getZYpp()->pool().eraseRequestedLocale( _zyppLang );
    } else {
        zypp::getZYpp()->pool().addRequestedLocale( _zyppLang );
    }
    setStatusIcon();
}

#include "YQPkgLangList.moc"
