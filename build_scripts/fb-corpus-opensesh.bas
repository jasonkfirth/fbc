/'
    Project: FreeBASIC browser corpus
    File: fb-corpus-opensesh.bas
    Purpose: Run OpenSesh's MIDI reader and ProTracker exporter on one file.
    Responsibilities: Pass virtual file paths and write the conversion report.
    The OpenSesh model owns MIDI parsing and the exporter owns the MOD layout.
'/

#lang "fb"

#include once "music_export.bi"

Private Sub writeTrackReport(ByVal reportFile As Integer, ByRef summary As MidiSummary)
    Print #reportFile, "MIDI format: "; LTrim(Str(summary.formatNumber))
    Print #reportFile, "Tracks: "; LTrim(Str(summary.trackCount))
    Print #reportFile, "Notes: "; LTrim(Str(summary.noteCount))
    For trackIndex As Integer = 0 To summary.trackCount - 1
        Print #reportFile, "Track "; LTrim(Str(trackIndex + 1)); ": "; _
            LTrim(Str(summary.tracks(trackIndex).noteCount)); " notes"
    Next trackIndex
End Sub

Private Function convertMidi() As Integer
    ' Each browser request owns a fresh virtual filesystem and a MIDI model.
    Dim As Integer reportFile = FreeFile()
    If Open("/conversion.txt" For Output Access Write As #reportFile) <> 0 Then Return 3

    Dim As MidiSummary summary
    If midi_LoadSummary(summary, "/input.mid") = 0 Then
        Print #reportFile, summary.errorText
        Close #reportFile
        Return 2
    End If

    writeTrackReport reportFile, summary
    Print #reportFile,
    Dim As OseModExportReport report
    If musicExport_SaveMod(summary, "/converted.mod", report) = 0 Then
        Print #reportFile, report.errorText
        Close #reportFile
        Return 2
    End If

    Print #reportFile, "Patterns: "; LTrim(Str(report.patternCount))
    Print #reportFile, "Notes written: "; LTrim(Str(report.notesWritten))
    Print #reportFile, "Notes dropped: "; LTrim(Str(report.notesDropped))
    Print #reportFile, "Notes shortened: "; LTrim(Str(report.notesTruncated))
    Print #reportFile, "Voices reassigned: "; LTrim(Str(report.voiceSteals))
    Print #reportFile, "Notes moved by octaves: "; LTrim(Str(report.octaveFoldedNotes))
    Print #reportFile, "Tempo events clamped: "; LTrim(Str(report.tempoEventsClamped))
    Close #reportFile
    Print "OPENSESH_MOD_EXPORTED"
    Return 0
End Function

End convertMidi()

/' end of fb-corpus-opensesh.bas '/
