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

  File:	      YQPkgFilterTab.cc

  Author:     Stefan Hundhammer <sh@suse.de>

  Textdomain "qt-pkg"
  
/-*/


#include <vector>
#include <algorithm> // std::swap()

#include <QAction>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolButton>

#define YUILogComponent "qt-pkg"
#include "YUILog.h"

#include "YUIException.h"
#include "YQPkgFilterTab.h"
#include "YQPkgDiskUsageList.h"
#include "YQSignalBlocker.h"
#include "YQIconPool.h"
#include "YQi18n.h"
#include "utf8.h"

using std::vector;
typedef vector<YQPkgFilterPage *> YQPkgFilterPageVector;

#define SHOW_ONLY_IMPORTANT_PAGES	1
#define VIEW_BUTTON_LEFT		1


#define MARGIN 5 		// inner margin between 3D borders and content
#define TOP_EXTRA_MARGIN		3
#define SPLITTER_HALF_SPACING		2


struct YQPkgFilterTabPrivate
{
    YQPkgFilterTabPrivate()
	: baseClassWidgetStack(0)
	, outerSplitter(0)
	, leftPaneSplitter(0)
	, filtersWidgetStack(0)
	, diskUsageList(0)
	, rightPane(0)
	, viewButton(0)
	, closeButton(0)
	, tabContextMenu(0)
	, tabContextMenuPage(0)
	{}

    QStackedWidget *		baseClassWidgetStack;
    QSplitter *			outerSplitter;
    QSplitter *			leftPaneSplitter;
    QStackedWidget *		filtersWidgetStack;
    YQPkgDiskUsageList *	diskUsageList;
    QWidget *			rightPane;
    QPushButton *		viewButton;
    QToolButton *		closeButton;
    QMenu *			tabContextMenu;
    QAction *			actionMovePageLeft;
    QAction *			actionMovePageRight;
    QAction *			actionClosePage;
    YQPkgFilterPage *		tabContextMenuPage;
    YQPkgFilterPageVector	pages;
};




YQPkgFilterTab::YQPkgFilterTab( QWidget * parent )
    : QTabWidget( parent )
    , priv( new YQPkgFilterTabPrivate() )
{
    YUI_CHECK_NEW( priv );

    // Nasty hack: Find the base class's QStackedWidget in its widget tree so
    // we have a place to put our own widgets. Unfortunately, this is private
    // in the base class, but Qt lets us search the widget hierarchy by widget
    // type. 
    
    priv->baseClassWidgetStack = findChild<QStackedWidget*>();
    YUI_CHECK_PTR( priv->baseClassWidgetStack );

    // Nasty hack: Disconnect the base class from signals from its tab bar.
    // We will handle that signal on our own.

    disconnect( tabBar(), SIGNAL( currentChanged( int ) ), 0, 0 );

    
    //
    // Splitter that divides this widget into a left and a right pane
    //
    
    priv->outerSplitter = new QSplitter( Qt::Horizontal, this );
    YUI_CHECK_NEW( priv->outerSplitter );

    priv->outerSplitter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding,
						     QSizePolicy::Expanding ) );
    priv->baseClassWidgetStack->addWidget( priv->outerSplitter );

    
#if SHOW_ONLY_IMPORTANT_PAGES

    //
    // "View" and "Close" buttons
    //
    
    QWidget * buttonBox = new QWidget( this );
    YUI_CHECK_NEW( buttonBox );
    setCornerWidget( buttonBox, Qt::TopRightCorner );
    buttonBox->setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );

    QHBoxLayout * buttonBoxLayout = new QHBoxLayout( buttonBox );
    YUI_CHECK_NEW( buttonBoxLayout );
    buttonBox->setLayout( buttonBoxLayout );
    buttonBoxLayout->setContentsMargins( 0, 0, 0, 0 );

#if VIEW_BUTTON_LEFT
    
    // Translators: Button with pop-up menu to open a new page (very much like
    // in a web browser) with another package filter view or to switch to an
    // existing one if it's open already
    
    priv->viewButton = new QPushButton( _( "&View" ), this );
    YUI_CHECK_NEW( priv->viewButton );
    setCornerWidget( priv->viewButton, Qt::TopLeftCorner );
#else
    priv->viewButton = new QPushButton( _( "&View" ), buttonBox );
    YUI_CHECK_NEW( priv->viewButton );
    buttonBoxLayout->addWidget( priv->viewButton );
    
#endif // VIEW_BUTTON_LEFT

    QMenu * menu = new QMenu();
    YUI_CHECK_NEW( menu );
    priv->viewButton->setMenu( menu );
    menu->setTearOffEnabled( true );

    connect( menu, SIGNAL( triggered( QAction * ) ),
	     this, SLOT  ( showPage ( QAction * ) ) );


    priv->closeButton = new QToolButton( buttonBox );
    YUI_CHECK_NEW( priv->closeButton );
    buttonBoxLayout->addWidget( priv->closeButton );
    priv->closeButton->setIcon( YQIconPool::tabRemove() );
    priv->closeButton->setToolTip( _( "Close the current page" ) );

    connect( priv->closeButton, SIGNAL( clicked()          ),
	     this,		SLOT  ( closeCurrentPage() ) );
    
#endif // SHOW_ONLY_IMPORTANT_PAGES


    //
    // Splitter that divides the left pane into upper filters area and disk usage area
    //

    priv->leftPaneSplitter = new QSplitter( Qt::Vertical, priv->outerSplitter );
    YUI_CHECK_NEW( priv->leftPaneSplitter );

    
    //
    // Left pane content
    //

    priv->filtersWidgetStack = new QStackedWidget( priv->leftPaneSplitter );
    YUI_CHECK_NEW( priv->filtersWidgetStack );
    
    priv->diskUsageList = new YQPkgDiskUsageList( priv->leftPaneSplitter );
    YUI_CHECK_NEW( priv->diskUsageList );

    {
	QSplitter * sp = priv->leftPaneSplitter;
	sp->setStretchFactor( sp->indexOf( priv->filtersWidgetStack ), 1 );
	sp->setStretchFactor( sp->indexOf( priv->diskUsageList      ), 2 );


	// FIXME: Don't always hide the disk usage list
	QList<int> sizes;
	sizes << priv->leftPaneSplitter->height();
	sizes << 0;
	sp->setSizes( sizes );
    }


    //
    // Right pane
    //

    priv->rightPane = new QWidget( priv->outerSplitter );
    YUI_CHECK_NEW( priv->rightPane );

    
    //
    // Stretch factors for left and right pane
    //
    {
	QSplitter * sp = priv->outerSplitter;
	sp->setStretchFactor( sp->indexOf( priv->leftPaneSplitter ), 0 );
	sp->setStretchFactor( sp->indexOf( priv->rightPane        ), 1 );
    }


    // Set up connections

    connect( tabBar(), 	SIGNAL( currentChanged( int ) ),
	     this,	SLOT  ( showPage      ( int ) ) );

    tabBar()->installEventFilter( this ); // for tab context menu
    

    //
    // Cosmetics
    //

    priv->baseClassWidgetStack->setContentsMargins( MARGIN,			// left
						    MARGIN + TOP_EXTRA_MARGIN,	// top
						    MARGIN,			// right
						    MARGIN );			// bottom
    
    priv->leftPaneSplitter->setContentsMargins	( 0,				// left
						  0,				// top
						  SPLITTER_HALF_SPACING,	// right
						  0 );				// bottom
						
    // priv->rightPane->setContentsMargins() is set when widgets are added to the right pane
}


YQPkgFilterTab::~YQPkgFilterTab()
{
    for ( YQPkgFilterPageVector::iterator it = priv->pages.begin();
	  it != priv->pages.end();
	  ++it )
    {
	delete (*it);
    }

    priv->pages.clear();
}


QWidget *
YQPkgFilterTab::rightPane() const
{
    return priv->rightPane;
}


YQPkgDiskUsageList *
YQPkgFilterTab::diskUsageList() const
{
    return priv->diskUsageList;
}


void
YQPkgFilterTab::addPage( const QString &	pageLabel,
			 QWidget *		pageContent,
			 const QString &	internalName,
			 bool			showAlways )
{
    YQPkgFilterPage * page = new YQPkgFilterPage( pageLabel,
						  pageContent,
						  internalName,
						  showAlways );
    YUI_CHECK_NEW( page );
    
    priv->pages.push_back( page );
    priv->filtersWidgetStack->addWidget( pageContent );


    if ( priv->viewButton && priv->viewButton->menu() )
    {
	QAction * action = new QAction( pageLabel, this );
	YUI_CHECK_NEW( action );
	action->setData( qVariantFromValue( pageContent ) );
	
	priv->viewButton->menu()->addAction( action );
    }
    
#if SHOW_ONLY_IMPORTANT_PAGES
    if ( showAlways )
#endif
    {
	page->tabIndex = tabBar()->addTab( pageLabel );
	priv->closeButton->setEnabled( tabBar()->count() > 1 && page->closeEnabled );
    }
}


void
YQPkgFilterTab::showPage( QWidget * pageContent )
{
    YQPkgFilterPage * page = findPage( pageContent );
    YUI_CHECK_PTR( page );
    
    showPage( page );
}


void
YQPkgFilterTab::showPage( const QString & internalName )
{
    YQPkgFilterPage * page = findPage( internalName );
    YUI_CHECK_PTR( page );
    
    showPage( page );
}


void
YQPkgFilterTab::showPage( int tabIndex )
{
    YQPkgFilterPage * page = findPage( tabIndex );

    if ( page )
	showPage( page );
}


void
YQPkgFilterTab::showPage( QAction * action )
{
    if ( ! action )
	return;

    QWidget * pageContent = action->data().value<QWidget *>();
    showPage( pageContent );
}


void
YQPkgFilterTab::showPage( YQPkgFilterPage * page )
{
    YUI_CHECK_PTR( page );
    YQSignalBlocker sigBlocker( tabBar() );

    if ( page->tabIndex < 0 ) // No corresponding tab yet?
    {
	// Add a tab for that page
	page->tabIndex = tabBar()->addTab( page->label );
    }

    priv->filtersWidgetStack->setCurrentWidget( page->content );
    tabBar()->setCurrentIndex( page->tabIndex );
    priv->closeButton->setEnabled( tabBar()->count() > 1 && page->closeEnabled );

    emit currentChanged( page->content );
}


void
YQPkgFilterTab::closeCurrentPage()
{
    if ( tabBar()->count() > 1 )
    {
	int currentIndex = tabBar()->currentIndex();
	YQPkgFilterPage * currentPage = findPage( currentIndex );

	if ( currentPage )
	    currentPage->tabIndex = -1;

	tabBar()->removeTab( currentIndex );

	//
	// Adjust tab index of the active pages to the right of that page
	//
	
	for ( YQPkgFilterPageVector::iterator it = priv->pages.begin();
	      it != priv->pages.end();
	      ++it )
	{
	    YQPkgFilterPage * page = *it;

	    if ( page->tabIndex >= currentIndex )
		page->tabIndex--;
	}

	showPage( tabBar()->currentIndex() );
    }
}


YQPkgFilterPage *
YQPkgFilterTab::findPage( QWidget * pageContent )
{
    for ( YQPkgFilterPageVector::iterator it = priv->pages.begin();
	  it != priv->pages.end();
	  ++it )
    {
	if ( (*it)->content == pageContent )
	    return *it;
    }
    
    return 0;
}


YQPkgFilterPage *
YQPkgFilterTab::findPage( const QString & internalName )
{
    for ( YQPkgFilterPageVector::iterator it = priv->pages.begin();
	  it != priv->pages.end();
	  ++it )
    {
	if ( (*it)->id == internalName )
	    return *it;
    }

    return 0;
}


YQPkgFilterPage *
YQPkgFilterTab::findPage( int tabIndex )
{
    if ( tabIndex < 0 )
	return 0;
    
    for ( YQPkgFilterPageVector::iterator it = priv->pages.begin();
	  it != priv->pages.end();
	  ++it )
    {
	if ( (*it)->tabIndex == tabIndex )
	    return *it;
    }

    return 0;
}


int
YQPkgFilterTab::tabCount() const
{
    return tabBar()->count();
}


bool
YQPkgFilterTab::eventFilter ( QObject * watchedObj, QEvent * event )
{
    if ( watchedObj == tabBar() &&
	 event && event->type() == QEvent::MouseButtonPress )
    {
        QMouseEvent * mouseEvent = dynamic_cast<QMouseEvent *> (event);

        if ( mouseEvent && mouseEvent->button() == Qt::RightButton )
	{
	    return postTabContextMenu( mouseEvent->pos() );
	}
    }

    return QTabWidget::eventFilter( watchedObj, event );
}


bool
YQPkgFilterTab::postTabContextMenu( const QPoint & pos )
{
    int tabIndex = tabBar()->tabAt( pos );

    if ( tabIndex >= 0 ) // -1 means "no tab at that position"
    {
	priv->tabContextMenuPage = findPage( tabIndex );

	if ( priv->tabContextMenuPage )
	{
	    if ( ! priv->tabContextMenu )
	    {
		// On-demand menu creation

		priv->tabContextMenu = new QMenu( this );
		YUI_CHECK_NEW( priv->tabContextMenu );

		priv->actionMovePageLeft  = new QAction( _( "Move page &left"  ), this );
		YUI_CHECK_NEW( priv->actionMovePageLeft );

		connect( priv->actionMovePageLeft, 	SIGNAL( triggered() ),
			 this,				SLOT  ( contextMovePageLeft() ) );

		
		priv->actionMovePageRight = new QAction( _( "Move page &right" ), this );
		YUI_CHECK_NEW( priv->actionMovePageRight );

		connect( priv->actionMovePageRight, 	SIGNAL( triggered()   ),
			 this,				SLOT  ( contextMovePageRight() ) );

		
		priv->actionClosePage = new QAction( _( "&Close page" ), this );
		YUI_CHECK_NEW( priv->actionClosePage );
		
		connect( priv->actionClosePage, 	SIGNAL( triggered()   	),
			 this,				SLOT  ( contextClosePage() ) );

		
		priv->tabContextMenu->addAction( priv->actionMovePageLeft  );
		priv->tabContextMenu->addAction( priv->actionMovePageRight );
		priv->tabContextMenu->addAction( priv->actionClosePage     );
	    }

	    // Enable / disable actions
	    
	    priv->actionMovePageLeft->setEnabled( tabIndex > 0 );
	    priv->actionMovePageRight->setEnabled( tabIndex < ( tabBar()->count() - 1 ) );
	    priv->actionClosePage->setEnabled( tabBar()->count() > 0 && priv->tabContextMenuPage->closeEnabled );
	    
	    priv->tabContextMenu->popup( mapToGlobal( pos ) );
	    
	    return true; // event consumed - no further processing
	}
    }

    return false; // no tab at that position
}


void
YQPkgFilterTab::contextMovePageLeft()
{
    if ( priv->tabContextMenuPage )
    {
	int contextPageIndex = priv->tabContextMenuPage->tabIndex;
	int otherPageIndex   = contextPageIndex-1;

	if ( otherPageIndex >= 0 )
	{
	    swapTabs( priv->tabContextMenuPage, findPage( otherPageIndex ) );
	}
    }
}


void
YQPkgFilterTab::contextMovePageRight()
{
    if ( priv->tabContextMenuPage )
    {
	int contextPageIndex = priv->tabContextMenuPage->tabIndex;
	int otherPageIndex   = contextPageIndex+1;

	if ( otherPageIndex < tabBar()->count() )
	{
	    swapTabs( priv->tabContextMenuPage, findPage( otherPageIndex ) );
	}
    }
}


void
YQPkgFilterTab::swapTabs( YQPkgFilterPage * page1, YQPkgFilterPage * page2 )
{
    if ( ! page1 or ! page2 )
	return;
    
    int oldCurrentIndex = tabBar()->currentIndex();
    std::swap( page1->tabIndex, page2->tabIndex );
    tabBar()->setTabText( page1->tabIndex, page1->label );
    tabBar()->setTabText( page2->tabIndex, page2->label );
    
    yuiDebug() << "Swapping tabs \"" << toUTF8( page1->label )
	       << "\" and \"" << toUTF8( page2->label ) << "\""
	       << endl;

    
    // If one of the two pages was the currently displayed one,
    // make sure the same one is still displayed
	    
    if ( oldCurrentIndex == page1->tabIndex )
    {
	YQSignalBlocker sigBlocker( tabBar() );
	tabBar()->setCurrentIndex( page2->tabIndex );
    }
    else if ( oldCurrentIndex == page2->tabIndex )
    {
	YQSignalBlocker sigBlocker( tabBar() );
	tabBar()->setCurrentIndex( page1->tabIndex );
    }
}


void
YQPkgFilterTab::contextClosePage()
{
    if ( priv->tabContextMenuPage )
    {
	int pageIndex = priv->tabContextMenuPage->tabIndex;
	int oldCurrentIndex = tabBar()->currentIndex();
	tabBar()->removeTab( pageIndex );

	
	//
	// Adjust tab index of the active pages to the right of that page
	//
	
	for ( YQPkgFilterPageVector::iterator it = priv->pages.begin();
	      it != priv->pages.end();
	      ++it )
	{
	    YQPkgFilterPage * page = *it;

	    if ( page->tabIndex >= pageIndex )
		page->tabIndex--;
	}

	if ( oldCurrentIndex == pageIndex )       // this page was the current page
	    showPage( tabBar()->currentIndex() ); // display the new current page
    }
}



#include "YQPkgFilterTab.moc"
