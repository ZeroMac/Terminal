/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "gdirenderer.hpp"

#pragma hdrstop

using namespace Microsoft::Console::Render;

// Routine Description:
// - Notifies us that the system has requested a particular pixel area of the client rectangle should be redrawn. (On WM_PAINT)
// Arguments:
// - prcDirtyClient - Pointer to pixel area (RECT) of client region the system believes is dirty
// Return Value:
// - HRESULT S_OK, GDI-based error code, or safemath error
HRESULT GdiEngine::InvalidateSystem(_In_ const RECT* const prcDirtyClient)
{
    RETURN_HR(_InvalidCombine(prcDirtyClient));
}

// Routine Description:
// - Notifies us that the console is attempting to scroll the existing screen area
// Arguments:
// - pcoordDelta - Pointer to character dimension (COORD) of the distance the console would like us to move while scrolling.
// Return Value:
// - HRESULT S_OK, GDI-based error code, or safemath error
HRESULT GdiEngine::InvalidateScroll(_In_ const COORD* const pcoordDelta)
{
    if (pcoordDelta->X != 0 || pcoordDelta->Y != 0)
    {
        POINT ptDelta = { 0 };
        RETURN_IF_FAILED(_ScaleByFont(pcoordDelta, &ptDelta));

        RETURN_IF_FAILED(_InvalidOffset(&ptDelta));

        SIZE szInvalidScrollNew;
        RETURN_IF_FAILED(LongAdd(_szInvalidScroll.cx, ptDelta.x, &szInvalidScrollNew.cx));
        RETURN_IF_FAILED(LongAdd(_szInvalidScroll.cy, ptDelta.y, &szInvalidScrollNew.cy));

        // Store if safemath succeeded
        _szInvalidScroll = szInvalidScrollNew;
    }

    return S_OK;
}

// Routine Description:
// - Notifies us that the console has changed the selection region and would like it updated
// Arguments:
// - rgsrSelection - Array of character region rectangles (one per line) that represent the selected area
// - cRectangles - Length of the array above.
// Return Value:
// - HRESULT S_OK or GDI-based error code
HRESULT GdiEngine::InvalidateSelection(_In_reads_(cRectangles) SMALL_RECT* const rgsrSelection, _In_ UINT const cRectangles)
{
    // Get the currently selected area as a GDI region
    wil::unique_hrgn hrgnSelection(CreateRectRgn(0, 0, 0, 0));
    RETURN_LAST_ERROR_IF_NULL(hrgnSelection.get());

    RETURN_IF_FAILED(_PaintSelectionCalculateRegion(rgsrSelection, cRectangles, hrgnSelection.get()));

    // XOR against the region we saved from the last time we rendered to find out what to invalidate
    // This is the space that needs to be inverted to either select or deselect the existing region into the new one.
    wil::unique_hrgn hrgnInvalid(CreateRectRgn(0, 0, 0, 0));
    RETURN_LAST_ERROR_IF_NULL(hrgnInvalid.get());

    int const iCombineResult = CombineRgn(hrgnInvalid.get(), _hrgnGdiPaintedSelection, hrgnSelection.get(), RGN_XOR);

    if (NULLREGION != iCombineResult && ERROR != iCombineResult)
    {
        // Invalidate that.
        RETURN_IF_FAILED(_InvalidateRgn(hrgnInvalid.get()));
    }

    return S_OK;
}

// Routine Description:
// - Notifies us that the console has changed the character region specified.
// - NOTE: This typically triggers on cursor or text buffer changes
// Arguments:
// - psrRegion - Character region (SMALL_RECT) that has been changed
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
HRESULT GdiEngine::Invalidate(const SMALL_RECT* const psrRegion)
{
    RECT rcRegion = { 0 };
    RETURN_IF_FAILED(_ScaleByFont(psrRegion, &rcRegion));
    RETURN_HR(_InvalidateRect(&rcRegion));
}

// Routine Description:
// - Notifies to repaint everything.
// - NOTE: Use sparingly. Only use when something that could affect the entire frame simultaneously occurs.
// Arguments:
// - <none>
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
HRESULT GdiEngine::InvalidateAll()
{
    RECT rc;
    RETURN_LAST_ERROR_IF_FALSE(GetClientRect(_hwndTargetWindow, &rc));
    RETURN_HR(InvalidateSystem(&rc));
}

// Routine Description:
// - Helper to combine the given rectangle into the invalid region to be updated on the next paint
// Arguments:
// - prc - Pixel region (RECT) that should be repainted on the next frame
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
HRESULT GdiEngine::_InvalidCombine(_In_ const RECT* const prc)
{
    if (!_fInvalidRectUsed)
    {
        _rcInvalid = *prc;
        _fInvalidRectUsed = true;
    }
    else
    {
        _OrRect(&_rcInvalid, prc);
    }

    // Ensure invalid areas remain within bounds of window.
    RETURN_IF_FAILED(_InvalidRestrict());

    return S_OK;
}

// Routine Description:
// - Helper to adjust the invalid region by the given offset such as when a scroll operation occurs.
// Arguments:
// - ppt - Distances by which we should move the invalid region in response to a scroll
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
HRESULT GdiEngine::_InvalidOffset(_In_ const POINT* const ppt)
{
    if (_fInvalidRectUsed)
    {
        RECT rcInvalidNew;

        RETURN_IF_FAILED(LongAdd(_rcInvalid.left, ppt->x, &rcInvalidNew.left));
        RETURN_IF_FAILED(LongAdd(_rcInvalid.right, ppt->x, &rcInvalidNew.right));
        RETURN_IF_FAILED(LongAdd(_rcInvalid.top, ppt->y, &rcInvalidNew.top));
        RETURN_IF_FAILED(LongAdd(_rcInvalid.bottom, ppt->y, &rcInvalidNew.bottom));

        // If all math succeeded, store the new invalid rect.
        _rcInvalid = rcInvalidNew;

        // Ensure invalid areas remain within bounds of window.
        RETURN_IF_FAILED(_InvalidRestrict());
    }

    return S_OK;
}

// Routine Description:
// - Helper to ensure the invalid region remains within the bounds of the window.
// Arguments:
// - <none>
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
HRESULT GdiEngine::_InvalidRestrict()
{
    // Ensure that the invalid area remains within the bounds of the client area
    RECT rcClient;

    // Do restriction only if retrieving the client rect was successful.
    RETURN_LAST_ERROR_IF_FALSE(GetClientRect(_hwndTargetWindow, &rcClient));

    _rcInvalid.left = max(_rcInvalid.left, rcClient.left);
    _rcInvalid.right = min(_rcInvalid.right, rcClient.right);
    _rcInvalid.top = max(_rcInvalid.top, rcClient.top);
    _rcInvalid.bottom = min(_rcInvalid.bottom, rcClient.bottom);

    return S_OK;
}

// Routine Description:
// - Helper to add a pixel rectangle to the invalid area
// Arguments:
// - prc - Pointer to pixel rectangle representing invalid area to add to next paint frame
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
HRESULT GdiEngine::_InvalidateRect(_In_ const RECT* const prc)
{
    RETURN_HR(_InvalidCombine(prc));
}

// Routine Description:
// - Helper to add a pixel region to the invalid area
// Arguments:
// - hrgn - Handle to pixel region representing invalid area to add to next paint frame
// Return Value:
// - S_OK, GDI related failure, or safemath failure.
HRESULT GdiEngine::_InvalidateRgn(_In_ HRGN hrgn)
{
    RECT rcInvalid;
    RETURN_LAST_ERROR_IF_FALSE(GetRgnBox(hrgn, &rcInvalid));
    RETURN_HR(_InvalidateRect(&rcInvalid));
}
