/********************************************************
 *                                                       *
 *   Copyright (C) Microsoft. All rights reserved.       *
 *                                                       *
 ********************************************************/

#include "pch.h"
#include "Profile.h"
#include "../../types/inc/Utils.hpp"
#include <DefaultSettings.h>

using namespace Microsoft::Terminal::TerminalApp;
using namespace winrt::Microsoft::Terminal::Settings;
using namespace winrt::Microsoft::Terminal::TerminalApp;
using namespace winrt::Windows::Data::Json;
using namespace ::Microsoft::Console;


static const std::wstring NAME_KEY{ L"name" };
static const std::wstring GUID_KEY{ L"guid" };
static const std::wstring COLORSCHEME_KEY{ L"colorscheme" };

static const std::wstring FOREGROUND_KEY{ L"foreground" };
static const std::wstring BACKGROUND_KEY{ L"background" };
static const std::wstring COLORTABLE_KEY{ L"colorTable" };
static const std::wstring HISTORYSIZE_KEY{ L"historySize" };
static const std::wstring SNAPONINPUT_KEY{ L"snapOnInput" };

static const std::wstring COMMANDLINE_KEY{ L"commandline" };
static const std::wstring FONTFACE_KEY{ L"fontFace" };
static const std::wstring FONTSIZE_KEY{ L"fontSize" };
static const std::wstring ACRYLICTRANSPARENCY_KEY{ L"acrylicOpacity" };
static const std::wstring USEACRYLIC_KEY{ L"useAcrylic" };
static const std::wstring SHOWSCROLLBARS_KEY{ L"showScrollbars" };

Profile::Profile() :
    _guid{},
    _name{ L"Default" },
    _schemeName{},

    _defaultForeground{  },
    _defaultBackground{  },
    _colorTable{},
    _historySize{ DEFAULT_HISTORY_SIZE },
    _snapOnInput{ true },

    _commandline{ L"cmd.exe" },
    _fontFace{ DEFAULT_FONT_FACE },
    _fontSize{ DEFAULT_FONT_SIZE },
    _acrylicTransparency{ 0.5 },
    _useAcrylic{ false },
    _showScrollbars{ true }
{
    UuidCreate(&_guid);
}

Profile::~Profile()
{

}

GUID Profile::GetGuid() const noexcept
{
    return _guid;
}

// Function Description:
// - Searches a list of color schemes to find one matching the given name. Will
//return the first match in the list, if the list has multiple schemes with the same name.
// Arguments:
// - schemes: a list of schemes to search
// - schemeName: the name of the sceme to look for
// Return Value:
// - a non-ownership pointer to the matching scheme if we found one, else nullptr
const ColorScheme* _FindScheme(const std::vector<ColorScheme>& schemes,
                               const std::wstring& schemeName)
{
    for (auto& scheme : schemes)
    {
        if (scheme.GetName() == schemeName)
        {
            return &scheme;
        }
    }
    return nullptr;
}

// Method Description:
// - Create a TerminalSettings from this object. Apply our settings, as well as
//      any colors from our colorscheme, if we have one.
// Arguments:
// - schemes: a list of schemes to look for our color scheme in, if we have one.
// Return Value:
// - a new TerminalSettings object with our settings in it.
TerminalSettings Profile::CreateTerminalSettings(const std::vector<ColorScheme>& schemes) const
{
    TerminalSettings terminalSettings{};

    // Fill in the Terminal Setting's CoreSettings from the profile
    for (int i = 0; i < _colorTable.size(); i++)
    {
        terminalSettings.SetColorTableEntry(i, _colorTable[i]);
    }
    terminalSettings.HistorySize(_historySize);
    terminalSettings.SnapOnInput(_snapOnInput);

    // Fill in the remaining properties from the profile
    terminalSettings.UseAcrylic(_useAcrylic);
    terminalSettings.TintOpacity(_acrylicTransparency);

    terminalSettings.FontFace(_fontFace);
    terminalSettings.FontSize(_fontSize);

    terminalSettings.Commandline(winrt::to_hstring(_commandline.c_str()));

    if (_schemeName)
    {
        const ColorScheme* const matchingScheme = _FindScheme(schemes, _schemeName.value());
        if (matchingScheme)
        {
            matchingScheme->ApplyScheme(terminalSettings);
        }
    }
    if (_defaultForeground)
    {
        terminalSettings.DefaultForeground(_defaultForeground.value());
    }
    if (_defaultBackground)
    {
        terminalSettings.DefaultBackground(_defaultBackground.value());
    }

    return terminalSettings;
}

// Method Description:
// - Serialize this object to a JsonObject.
// Arguments:
// - <none>
// Return Value:
// - a JsonObject which is an equivalent serialization of this object.
JsonObject Profile::ToJson() const
{
    winrt::Windows::Data::Json::JsonObject jsonObject;

    // Profile-specific settings
    const auto guidStr = Utils::GuidToString(_guid);
    const auto guid = JsonValue::CreateStringValue(guidStr);
    const auto name = JsonValue::CreateStringValue(_name);

    // Core Settings
    const auto historySize = JsonValue::CreateNumberValue(_historySize);
    const auto snapOnInput = JsonValue::CreateBooleanValue(_snapOnInput);

    // Control Settings
    const auto cmdline = JsonValue::CreateStringValue(_commandline);
    const auto fontFace = JsonValue::CreateStringValue(_fontFace);
    const auto fontSize = JsonValue::CreateNumberValue(_fontSize);
    const auto acrylicTransparency = JsonValue::CreateNumberValue(_acrylicTransparency);
    const auto useAcrylic = JsonValue::CreateBooleanValue(_useAcrylic);
    const auto showScrollbars = JsonValue::CreateBooleanValue(_showScrollbars);

    jsonObject.Insert(GUID_KEY, guid);
    jsonObject.Insert(NAME_KEY, name);

    // Core Settings
    if (_defaultForeground)
    {
        const auto defaultForeground = JsonValue::CreateStringValue(Utils::ColorToHexString(_defaultForeground.value()));
        jsonObject.Insert(FOREGROUND_KEY, defaultForeground);
    }
    if (_defaultBackground)
    {
        const auto defaultBackground = JsonValue::CreateStringValue(Utils::ColorToHexString(_defaultBackground.value()));
        jsonObject.Insert(BACKGROUND_KEY, defaultBackground);
    }
    if (_schemeName)
    {
        const auto scheme = JsonValue::CreateStringValue(_schemeName.value());
        jsonObject.Insert(COLORSCHEME_KEY, scheme);
    }
    else
    {
        JsonArray tableArray{};
        for (auto& color : _colorTable)
        {
            auto s = Utils::ColorToHexString(color);
            tableArray.Append(JsonValue::CreateStringValue(s));
        }

        jsonObject.Insert(COLORTABLE_KEY, tableArray);

    }
    jsonObject.Insert(HISTORYSIZE_KEY, historySize);
    jsonObject.Insert(SNAPONINPUT_KEY, snapOnInput);

    // Control Settings
    jsonObject.Insert(COMMANDLINE_KEY, cmdline);
    jsonObject.Insert(FONTFACE_KEY, fontFace);
    jsonObject.Insert(FONTSIZE_KEY, fontSize);
    jsonObject.Insert(ACRYLICTRANSPARENCY_KEY, acrylicTransparency);
    jsonObject.Insert(USEACRYLIC_KEY, useAcrylic);
    jsonObject.Insert(SHOWSCROLLBARS_KEY, showScrollbars);

    return jsonObject;
}

// Method Description:
// - Create a new instance of this class from a serialized JsonObject.
// Arguments:
// - json: an object which should be a serialization of a Profile object.
// Return Value:
// - a new Profile instance created from the values in `json`
Profile Profile::FromJson(winrt::Windows::Data::Json::JsonObject json)
{
    Profile result{};

    // Profile-specific Settings
    if (json.HasKey(NAME_KEY))
    {
        result._name = json.GetNamedString(NAME_KEY);
    }
    if (json.HasKey(GUID_KEY))
    {
        const auto guidString = json.GetNamedString(GUID_KEY);
        // TODO: MSFT:20737698 - if this fails, display an approriate error
        const auto guid = Utils::GuidFromString(guidString.c_str());
        result._guid = guid;
    }

    // Core Settings
    if (json.HasKey(FOREGROUND_KEY))
    {
        const auto fgString = json.GetNamedString(FOREGROUND_KEY);
        // TODO: MSFT:20737698 - if this fails, display an approriate error
        const auto color = Utils::ColorFromHexString(fgString.c_str());
        result._defaultForeground = color;
    }
    if (json.HasKey(BACKGROUND_KEY))
    {
        const auto bgString = json.GetNamedString(BACKGROUND_KEY);
        // TODO: MSFT:20737698 - if this fails, display an approriate error
        const auto color = Utils::ColorFromHexString(bgString.c_str());
        result._defaultBackground = color;
    }
    if (json.HasKey(COLORSCHEME_KEY))
    {
        result._schemeName = json.GetNamedString(COLORSCHEME_KEY);
    }
    else
    {
        if (json.HasKey(COLORTABLE_KEY))
        {
            const auto table = json.GetNamedArray(COLORTABLE_KEY);
            int i = 0;
            for (auto v : table)
            {
                if (v.ValueType() == JsonValueType::String)
                {
                    const auto str = v.GetString();
                    // TODO: MSFT:20737698 - if this fails, display an approriate error
                    const auto color = Utils::ColorFromHexString(str.c_str());
                    result._colorTable[i] = color;
                }
                i++;
            }
        }
    }
    if (json.HasKey(HISTORYSIZE_KEY))
    {
        // TODO:MSFT:20642297 - Use a sentinel value (-1) for "Infinite scrollback"
        result._historySize = static_cast<int32_t>(json.GetNamedNumber(HISTORYSIZE_KEY));
    }
    if (json.HasKey(SNAPONINPUT_KEY))
    {
        result._snapOnInput = json.GetNamedBoolean(SNAPONINPUT_KEY);
    }

    // Control Settings
    if (json.HasKey(COMMANDLINE_KEY))
    {
        result._commandline = json.GetNamedString(COMMANDLINE_KEY);
    }
    if (json.HasKey(FONTFACE_KEY))
    {
        result._fontFace = json.GetNamedString(FONTFACE_KEY);
    }
    if (json.HasKey(FONTSIZE_KEY))
    {
        result._fontSize = static_cast<int32_t>(json.GetNamedNumber(FONTSIZE_KEY));
    }
    if (json.HasKey(ACRYLICTRANSPARENCY_KEY))
    {
        result._acrylicTransparency = json.GetNamedNumber(ACRYLICTRANSPARENCY_KEY);
    }
    if (json.HasKey(USEACRYLIC_KEY))
    {
        result._useAcrylic = json.GetNamedBoolean(USEACRYLIC_KEY);
    }
    if (json.HasKey(SHOWSCROLLBARS_KEY))
    {
        result._showScrollbars = json.GetNamedBoolean(SHOWSCROLLBARS_KEY);
    }

    return result;
}



void Profile::SetFontFace(std::wstring fontFace) noexcept
{
    _fontFace = fontFace;
}

void Profile::SetColorScheme(std::optional<std::wstring> schemeName) noexcept
{
    _schemeName = schemeName;
}

void Profile::SetAcrylicOpacity(double opacity) noexcept
{
    _acrylicTransparency = opacity;
}

void Profile::SetCommandline(std::wstring cmdline) noexcept
{
    _commandline = cmdline;
}

void Profile::SetName(std::wstring name) noexcept
{
    _name = name;
}

void Profile::SetUseAcrylic(bool useAcrylic) noexcept
{
    _useAcrylic = useAcrylic;
}

void Profile::SetDefaultForeground(COLORREF defaultForeground) noexcept
{
    _defaultForeground = defaultForeground;
}

void Profile::SetDefaultBackground(COLORREF defaultBackground) noexcept
{
    _defaultBackground = defaultBackground;
}

// Method Description:
// - Returns the name of this profile.
// Arguments:
// - <none>
// Return Value:
// - the name of this profile
std::wstring_view Profile::GetName() const noexcept
{
    return _name;
}
