#pragma once

#include <SKSEMenuFramework.h>

namespace ImGuiMCP {
    // Backwards compatibility alias: previous versions of SKSEMenuFramework nested ImGui under ImGuiMCP
    namespace ImGui = ImGuiMCP;

    // Out-pointer overloads for functions that return by value in newer SKSEMenuFramework
    inline void CalcTextSize(ImVec2* pOut, const char* text, const char* text_end = NULL, bool hide_text_after_double_hash = false, float wrap_width = -1.0f) {
        if (pOut) *pOut = CalcTextSize(text, text_end, hide_text_after_double_hash, wrap_width);
    }
    inline void GetItemRectMax(ImVec2* pOut) {
        if (pOut) *pOut = GetItemRectMax();
    }
    inline void GetItemRectMin(ImVec2* pOut) {
        if (pOut) *pOut = GetItemRectMin();
    }
    inline void GetItemRectSize(ImVec2* pOut) {
        if (pOut) *pOut = GetItemRectSize();
    }
    inline void GetContentRegionAvail(ImVec2* pOut) {
        if (pOut) *pOut = GetContentRegionAvail();
    }
    inline void GetContentRegionMax(ImVec2* pOut) {
        if (pOut) *pOut = GetContentRegionMax();
    }
    inline void GetWindowContentRegionMin(ImVec2* pOut) {
        if (pOut) *pOut = GetWindowContentRegionMin();
    }
    inline void GetWindowContentRegionMax(ImVec2* pOut) {
        if (pOut) *pOut = GetWindowContentRegionMax();
    }
    inline void GetWindowPos(ImVec2* pOut) {
        if (pOut) *pOut = GetWindowPos();
    }
    inline void GetWindowSize(ImVec2* pOut) {
        if (pOut) *pOut = GetWindowSize();
    }
    inline void GetCursorPos(ImVec2* pOut) {
        if (pOut) *pOut = GetCursorPos();
    }
    inline void GetCursorStartPos(ImVec2* pOut) {
        if (pOut) *pOut = GetCursorStartPos();
    }
    inline void GetCursorScreenPos(ImVec2* pOut) {
        if (pOut) *pOut = GetCursorScreenPos();
    }
    inline void GetMousePos(ImVec2* pOut) {
        if (pOut) *pOut = GetMousePos();
    }
}
