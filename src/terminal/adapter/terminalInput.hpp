/*++
Copyright (c) Microsoft Corporation

Module Name:
- terminalInput.hpp

Abstract:
- This serves as an adapter between virtual key input from a user and the virtual terminal sequences that are
  typically emitted by an xterm-compatible console.

Author(s):
- Michael Niksa (MiNiksa) 30-Oct-2015
--*/
#pragma once

namespace Microsoft
{
    namespace Console
    {
        namespace VirtualTerminal
        {
            typedef void(*WriteInputEvents)(_In_reads_(cInput) INPUT_RECORD* rgInput, _In_ DWORD cInput);


            class TerminalInput sealed
            {
            public:
                TerminalInput(_In_ WriteInputEvents const pfnWriteEvents);
                ~TerminalInput();

                bool HandleKey(_In_ const INPUT_RECORD* const pInput) const;
                void ChangeKeypadMode(_In_ bool const fApplicationMode);
                void ChangeCursorKeysMode(_In_ bool const fApplicationMode);

            private:
                WriteInputEvents _pfnWriteEvents;
                bool _fKeypadApplicationMode = false;
                bool _fCursorApplicationMode = false;

                void _SendNullInputSequence(_In_ DWORD const dwControlKeyState) const;
                void _SendInputSequence(_In_ PCWSTR const pwszSequence) const;
                void _SendEscapedInputSequence(_In_ const wchar_t wch) const;

                struct _TermKeyMap
                {
                    WORD const wVirtualKey;
                    PCWSTR const pwszSequence;

                    static const size_t s_cchMaxSequenceLength;

                    _TermKeyMap(_In_ WORD const wVirtualKey, _In_ PCWSTR const pwszSequence) :
                        wVirtualKey(wVirtualKey),
                        pwszSequence(pwszSequence) {};

                    // C++11 syntax for prohibiting assignment
                    // We can't assign, everything here is const.
                    // We also shouldn't need to, this is only for a specific table.
                    _TermKeyMap& operator=(const _TermKeyMap&) = delete;
                };

                static const _TermKeyMap s_rgCursorKeysNormalMapping[];
                static const _TermKeyMap s_rgCursorKeysApplicationMapping[];
                static const _TermKeyMap s_rgKeypadNumericMapping[];
                static const _TermKeyMap s_rgKeypadApplicationMapping[];
                static const _TermKeyMap s_rgModifierKeyMapping[];
            
                static const size_t s_cCursorKeysNormalMapping;
                static const size_t s_cCursorKeysApplicationMapping;
                static const size_t s_cKeypadNumericMapping;
                static const size_t s_cKeypadApplicationMapping;
                static const size_t s_cModifierKeyMapping;

                static bool s_IsShiftPressed(_In_ const KEY_EVENT_RECORD* const pKeyEvent);
                static bool s_IsAltPressed(_In_ const KEY_EVENT_RECORD* const pKeyEvent);
                static bool s_IsCtrlPressed(_In_ const KEY_EVENT_RECORD* const pKeyEvent);
                static bool s_IsModifierPressed(_In_ const KEY_EVENT_RECORD* const pKeyEvent);
                static bool s_IsCursorKey(_In_ const KEY_EVENT_RECORD* const pKeyEvent);
                bool _SearchKeyMapping(_In_ const KEY_EVENT_RECORD* const pKeyEvent,
                                       _In_reads_(cKeyMapping) const TerminalInput::_TermKeyMap* keyMapping,
                                       _In_ size_t const cKeyMapping,
                                       _Out_ const TerminalInput::_TermKeyMap** pMatchingMapping) const;
                bool _TranslateDefaultMapping(_In_ const KEY_EVENT_RECORD* const pKeyEvent,
                                              _In_reads_(cKeyMapping) const TerminalInput::_TermKeyMap* keyMapping,
                                              _In_ size_t const cKeyMapping) const;
                bool _SearchWithModifier(_In_ const KEY_EVENT_RECORD* const pKeyEvent) const;

            public:
                const size_t GetKeyMappingLength(_In_ const KEY_EVENT_RECORD* const pKeyEvent) const;
                const _TermKeyMap* GetKeyMapping(_In_ const KEY_EVENT_RECORD* const pKeyEvent) const;

            };
        };
    };
};