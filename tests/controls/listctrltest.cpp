///////////////////////////////////////////////////////////////////////////////
// Name:        tests/controls/listctrltest.cpp
// Purpose:     wxListCtrl unit test
// Author:      Vadim Zeitlin
// Created:     2008-11-26
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwidgets.org>
//              (c) 2010 Steven Lamerton
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "testprec.h"

#if wxUSE_LISTCTRL


#ifndef WX_PRECOMP
    #include "wx/app.h"
#endif // WX_PRECOMP

#include "wx/listctrl.h"
#include "wx/artprov.h"
#include "wx/imaglist.h"
#include "listbasetest.h"
#include "testableframe.h"
#include "wx/uiaction.h"

// ----------------------------------------------------------------------------
// test class
// ----------------------------------------------------------------------------

class ListCtrlTestCase : public ListBaseTestCase
{
public:
    ListCtrlTestCase();
    virtual ~ListCtrlTestCase() override;

    virtual wxListCtrl *GetList() const override { return m_list; }

protected:
    wxListCtrl *m_list;

    wxDECLARE_NO_COPY_CLASS(ListCtrlTestCase);
};

// ----------------------------------------------------------------------------
// test initialization
// ----------------------------------------------------------------------------

ListCtrlTestCase::ListCtrlTestCase()
{
    m_list = new wxListCtrl(wxTheApp->GetTopWindow());
    m_list->SetWindowStyle(wxLC_REPORT | wxLC_EDIT_LABELS);
    m_list->SetSize(400, 200);

    wxTheApp->GetTopWindow()->Raise();
}

ListCtrlTestCase::~ListCtrlTestCase()
{
    DeleteTestWindow(m_list);
}

wxLIST_BASE_TESTS(ListCtrl, "[listctrl]")

// Note that wxLIST_BASE_TESTS() already defines "ListCtrl::EditLabel" test.
TEST_CASE_METHOD(ListCtrlTestCase, "ListCtrl::CallEditLabel", "[listctrl]")
{
    EventCounter editItem(m_list, wxEVT_LIST_BEGIN_LABEL_EDIT);
    EventCounter endEditItem(m_list, wxEVT_LIST_END_LABEL_EDIT);

    m_list->InsertColumn(0, "Column 0");
    m_list->InsertItem(0, "foo");
    m_list->EditLabel(0);

    m_list->EndEditLabel(true);

    CHECK(editItem.GetCount() == 1);
    CHECK(endEditItem.GetCount() == 1);
}

TEST_CASE_METHOD(ListCtrlTestCase, "ListCtrl::SubitemRect", "[listctrl]")
{
    wxBitmap bmp = wxArtProvider::GetBitmap(wxART_ERROR);

    wxImageList* const iml = new wxImageList(bmp.GetWidth(), bmp.GetHeight());
    iml->Add(bmp);
    m_list->AssignImageList(iml, wxIMAGE_LIST_SMALL);

    m_list->InsertColumn(0, "Column 0");
    m_list->InsertColumn(1, "Column 1");
    m_list->InsertColumn(2, "Column 2");
    for ( int i = 0; i < 3; i++ )
    {
        long index = m_list->InsertItem(i, wxString::Format("This is item %d", i), 0);
        m_list->SetItem(index, 1, wxString::Format("Column 1 item %d", i));
        m_list->SetItem(index, 2, wxString::Format("Column 2 item %d", i));
    }

    wxRect rectLabel, rectIcon, rectItem;

    // First check a subitem with an icon: it should have a valid icon
    // rectangle and the label rectangle should be adjacent to it.
    m_list->GetSubItemRect(1, 0, rectItem, wxLIST_RECT_BOUNDS);
    m_list->GetSubItemRect(1, 0, rectIcon, wxLIST_RECT_ICON);
    m_list->GetSubItemRect(1, 0, rectLabel, wxLIST_RECT_LABEL);

    CHECK(!rectIcon.IsEmpty());
    // Note that we can't use "==" here, in the native MSW version there is a
    // gap between the item rectangle and the icon one.
    CHECK(rectIcon.GetLeft() >= rectItem.GetLeft());
    CHECK(rectLabel.GetLeft() == rectIcon.GetRight() + 1);
    CHECK(rectLabel.GetRight() == rectItem.GetRight());

    // For a subitem without an icon, label rectangle is the same one as the
    // entire item one and the icon rectangle should be empty.
    m_list->GetSubItemRect(1, 1, rectItem, wxLIST_RECT_BOUNDS);
    m_list->GetSubItemRect(1, 1, rectIcon, wxLIST_RECT_ICON);
    m_list->GetSubItemRect(1, 1, rectLabel, wxLIST_RECT_LABEL);

    CHECK(rectIcon.IsEmpty());
    // Here we can't check for exact equality either as there can be a margin.
    CHECK(rectLabel.GetLeft() >= rectItem.GetLeft());
    CHECK(rectLabel.GetRight() == rectItem.GetRight());
}

TEST_CASE_METHOD(ListCtrlTestCase, "ListCtrl::ColumnCount", "[listctrl]")
{
    CHECK(m_list->GetColumnCount() == 0);
    m_list->InsertColumn(0, "Column 0");
    m_list->InsertColumn(1, "Column 1");
    CHECK(m_list->GetColumnCount() == 2);

    // Recreate the control in other modes to check the count there as well.
    delete m_list;
    m_list = new wxListCtrl(wxTheApp->GetTopWindow(), wxID_ANY,
                            wxDefaultPosition, wxDefaultSize,
                            wxLC_LIST);
    CHECK(m_list->GetColumnCount() == 1);

    delete m_list;
    m_list = new wxListCtrl(wxTheApp->GetTopWindow(), wxID_ANY,
                            wxDefaultPosition, wxDefaultSize,
                            wxLC_ICON);
    CHECK(m_list->GetColumnCount() == 0);

    delete m_list;
    m_list = new wxListCtrl(wxTheApp->GetTopWindow(), wxID_ANY,
                            wxDefaultPosition, wxDefaultSize,
                            wxLC_SMALL_ICON);
    CHECK(m_list->GetColumnCount() == 0);
}

#if wxUSE_UIACTIONSIMULATOR

TEST_CASE_METHOD(ListCtrlTestCase, "ListCtrl::ColumnDrag", "[listctrl]")
{
    if ( !EnableUITests() )
        return;

    EventCounter begindrag(m_list, wxEVT_LIST_COL_BEGIN_DRAG);
    EventCounter dragging(m_list, wxEVT_LIST_COL_DRAGGING);
    EventCounter enddrag(m_list, wxEVT_LIST_COL_END_DRAG);

    m_list->InsertColumn(0, "Column 0");
    m_list->InsertColumn(1, "Column 1");
    m_list->InsertColumn(2, "Column 2");
    m_list->Update();
    m_list->SetFocus();

    wxUIActionSimulator sim;

    wxPoint pt = m_list->ClientToScreen(wxPoint(m_list->GetColumnWidth(0), 5));

    sim.MouseMove(pt);
    wxYield();

    sim.MouseDown();
    wxYield();

    sim.MouseMove(pt.x + 50, pt.y);
    wxYield();

    sim.MouseUp();
    wxYield();

    CHECK( begindrag.GetCount() == 1 );
    CHECK( dragging.GetCount() > 0 );
    CHECK( enddrag.GetCount() == 1 );

    m_list->ClearAll();
}

TEST_CASE_METHOD(ListCtrlTestCase, "ListCtrl::ColumnClick", "[listctrl]")
{
    if ( !EnableUITests() )
        return;

    EventCounter colclick(m_list, wxEVT_LIST_COL_CLICK);
    EventCounter colrclick(m_list, wxEVT_LIST_COL_RIGHT_CLICK);


    m_list->InsertColumn(0, "Column 0", wxLIST_FORMAT_LEFT, 60);

    wxUIActionSimulator sim;

    sim.MouseMove(m_list->ClientToScreen(wxPoint(4, 4)));
    wxYield();

    sim.MouseClick();
    sim.MouseClick(wxMOUSE_BTN_RIGHT);
    wxYield();

    CHECK( colclick.GetCount() == 1 );
    CHECK( colrclick.GetCount() == 1 );

    m_list->ClearAll();
}
#endif // wxUSE_UIACTIONSIMULATOR

#endif // wxUSE_LISTCTRL
