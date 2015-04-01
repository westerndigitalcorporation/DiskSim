Attribute VB_Name = "DiskSimModule"
Sub DiskSimAnalyzeAndCreatePlots()
    Dim startTime As Date
    Dim endTime As Date
    Dim analyzeTime As Date

    startTime = Time
    AnalyzeDiskSimData
    analyzeTime = Time
    ProcessPlots
    endTime = Time
    MsgBox ("Process Time (sec):" & DateDiff("s", startTime, endTime) & Chr(13) & "Analysis Time (sec):" & DateDiff("s", startTime, analyzeTime))
End Sub
'Version 0.2
Sub AnalyzeDiskSimData()
    Dim DiskSimFilesName As String
    Dim FName As Variant
    Dim parseFName() As String
    
    Dim SpreadSheetFileNames() As String
    Dim XGIGFileNames() As String
    Dim DiskSimFileNames() As String
    Dim numFiles As Integer
    
    Dim currentDir As String
    Dim XGIGSheetName As String
    Dim DiskSimSheetName As String
    Dim DiskSimFilesSheetName As String
    Dim StatsSheetName As String
    Dim arraySize As Integer
    Dim myStatus As Boolean
    
    Dim startTime As Date
    Dim endTime As Date
    
    startTime = Time
    Application.ScreenUpdating = False

    ActiveWorkbook.Author = "hurst_r"

    DiskSimFilesSheetName = "DiskSimFiles"
    StatsSheetName = "Stats"

    ' get the filenames to process
    If (True = GetDiskSimFilesToProcess(DiskSimFilesName, currentDir, XGIGSheetName, DiskSimSheetName, XGIGFileNames, DiskSimFileNames)) Then
        Exit Sub
    End If

    numFiles = GetUpper(XGIGFileNames) - 1
    ReDim SpreadSheetFileNames(numFiles)
    For Index = 0 To numFiles
        ' Get XGIG data\
        FName = XGIGFileNames(Index)
        parseFName = Split(FName, ".")
        SpreadSheetFileNames(Index) = parseFName(0) & ".xlsx"
        Application.StatusBar = "Creating " & SpreadSheetFileNames(Index)
        
        Workbooks.Add xlWBATWorksheet
        InsertWorkSheet (XGIGSheetName)
        ImportCsvFile FName, ActiveSheet.Cells(1, 1)
        If (CheckXGIGSheet(XGIGSheetName)) Then
            Exit Sub
        End If
        
        ' Get DiskSim data
        FName = DiskSimFileNames(Index)
        InsertWorkSheet (DiskSimSheetName)
        ImportCsvFile FName, ActiveSheet.Cells(1, 1)

        'Process the data
        If (CreateAndPopulateStatsWorkSheet(XGIGSheetName, DiskSimSheetName, StatsSheetName)) Then
            Exit Sub
        End If

        ' save the worksheet
        ActiveWorkbook.SaveAs FileName:=SpreadSheetFileNames(Index)
        ActiveWorkbook.Close
    Next Index
    endTime = Time
End Sub
Sub ProcessPlots()
    Dim DiskSimFilesName As Variant
    Dim currentDir As String
    Dim XGIGSheetName As String
    Dim DiskSimSheetName As String
    Dim XGIGFileNames() As String
    Dim DiskSimFileNames() As String

    Dim endFileIndex As Integer
    Dim ChartFileName As String
    Dim FName As Variant
    Dim lastFileRow As Integer
    Dim DiskSimFilesSheetName As String
    Dim StatsSheetName As String
    Dim parseFName() As String
    Dim SpreadSheetFileNames() As String
    Dim arraySize As Integer
    Dim myStatus As Boolean
    Dim startTime As Date
    Dim endTime As Date

    ActiveWorkbook.Author = "hurst_r"
    
    startTime = Time
    DiskSimFilesSheetName = "DiskSimFiles"
    StatsSheetName = "Stats"
    ChartFileName = "logsim_result_charts.xlsx"

    ' open the file containing the filenames to process
    If (True = GetDiskSimFilesToProcess(DiskSimFilesName, currentDir, XGIGSheetName, DiskSimSheetName, XGIGFileNames, DiskSimFileNames)) Then
        Exit Sub
    End If

    'create the spreadsheet filenames
    endFileIndex = GetUpper(XGIGFileNames) - 1
    ReDim SpreadSheetFileNames(endFileIndex)
    For Index = 0 To endFileIndex
        ' Get and parse filename
        FName = XGIGFileNames(Index)
        parseFName = Split(FName, ".")
        SpreadSheetFileNames(Index) = parseFName(0) & ".xlsx"
    Next Index

    myStatus = CopyStatsToChartsXlsx(ChartFileName, SpreadSheetFileNames, GetUpper(SpreadSheetFileNames), StatsSheetName)
    myStatus = CreateChartData(ChartFileName, "ChartData_XGIG", "ChartData_DiskSim", SpreadSheetFileNames, GetUpper(SpreadSheetFileNames))
    myStatus = PlotChartData(ChartFileName, "ChartData_XGIG", "ChartData_DiskSim", SpreadSheetFileNames, GetUpper(SpreadSheetFileNames))
End Sub
'Version 0.1
Sub CheckXGIGFile()
    Dim FName As Variant
    Dim parseFName() As String
    Dim XGIGSheetName As String

    ActiveWorkbook.Author = "hurst_r"
    
    ' get the filename to process
    FName = Application.GetOpenFilename("Text Files (*.csv), *.csv")
    If (False = FName) Then
        Exit Sub
    End If

    ' extract the filename
    parseFName = Split(FName, ".")
    XGIGSheetName = Mid(parseFName(0), InStrRev(parseFName(0), "\") + 1)

    InsertWorkSheet (XGIGSheetName)
    ImportCsvFile FName, ActiveSheet.Cells(1, 1)

    CheckXGIGSheet (XGIGSheetName)
End Sub
Function CheckXGIGSheet(myXGIGSheetName As String) As Boolean
    Dim lastDriveRow As Variant
    
    'Check validity of each file
    'Check Command column in drive (XGIG) trace
    lastDriveRow = Worksheets(myXGIGSheetName).Range("A:A").Cells.SpecialCells(xlCellTypeConstants).Count
    For i = 2 To lastDriveRow
        If (Not (("READ_10" = Worksheets(myXGIGSheetName).Cells(i, 25).Value) Or "WRITE_10" = Worksheets(myXGIGSheetName).Cells(i, 25).Value)) Then
            MsgBox ("XGIG Command Row Invalid: " & i & Chr(13) & "Value: " & Worksheets(myXGIGSheetName).Cells(i, 25).Value)
            CheckXGIGSheet = True
            Exit Function
        End If
        
        ' check the block xfer length field
        If (128 < Worksheets(myXGIGSheetName).Cells(i, 9).Value) Then
            MsgBox ("XGIG Block Xfer Length Invalid: " & i & Chr(13) & "Value: " & Worksheets(myXGIGSheetName).Cells(i, 9).Value)
            CheckXGIGSheet = True
            Exit Function
        End If
    Next i

    CheckXGIGSheet = False
End Function
Function EngAnd(parm1 As Long, parm2 As Long)
    EngAnd = parm1 And parm2
End Function
Sub InsertWorkSheet(wsName As String)
    Dim worksh As Integer
    Dim worksheetexists As Boolean

    worksh = Application.Sheets.Count
    worksheetexists = False

    For x = 1 To worksh
        If Worksheets(x).Name = wsName Then
            worksheetexists = True

            Application.DisplayAlerts = False
            Worksheets(x).Delete
            Application.DisplayAlerts = True
            Exit For
        End If
    Next x

    Worksheets.Add after:=Worksheets(Worksheets.Count)
    ActiveSheet.Name = wsName
End Sub
'False = OK
'True = Failed
Function CreateAndPopulateStatsWorkSheet(myXGIGSheetName As String, myDiskSimSheetName As String, myStatSheeetName As String) As Boolean
    Dim lastDriveRow As Variant
    Dim lastDiskSimRow As Variant
    Dim temp1 As Variant
    Dim temp2 As Variant
    Dim temp3 As Integer

    InsertWorkSheet (myStatSheeetName)
    ' MsgBox ("Active Sheet: " & ActiveSheet.Name)

    'Populate header across top
    Range("A1:E1").ColumnWidth = 16
    Range("A1:E1").Font.Bold = True
    Range("C1").Value = "Drive"
    Range("D1").Value = "DiskSim"
    Range("E1").Value = "%error"
    
    'Populate column A
    Range("E:E").NumberFormat = "0.00%"
    Range("A:A").ColumnWidth = 19
    Range("A:A").Font.Bold = True
    Range("A1").Value = "BSU"
    Range("A2").Value = "Total Requests"
    Range("A3").Value = "Total Time"
    Range("A4").Value = "IOPS"
    Range("A5").Value = "Total CCT(ms)"
    Range("A6").Value = "Total qCCT(ms)"
    Range("A7").Value = "ToD"
    Range("A10").Value = "IODriver Queue Depth"
    Range("A13").Value = "Disk Ctlr Queue Depth"
    Range("A16").Value = "Cache Hits"
    Range("A20").Value = "CCT(ms)"
    Range("A23").Value = "qCCT(ms)"

    'Populate column B
    Range("B:B").ColumnWidth = 17
    Range("B:B").Font.Bold = True

    Range("B7").Value = "min"
    Range("B8").Value = "max"
    Range("B9").Value = "avg"

    Range("B10").Value = "min"
    Range("B11").Value = "max"
    Range("B12").Value = "avg"

    Range("B13").Value = "min"
    Range("B14").Value = "max"
    Range("B15").Value = "avg"

    Range("B17").Value = "BUFFER_NOMATCH"
    Range("B18").Value = "BUFFER_WHOLE"
    Range("B19").Value = "BUFFER_PARTIAL"
    
    Range("B20").Value = "min"
    Range("B21").Value = "max"
    Range("B22").Value = "avg"

    Range("B23").Value = "min"
    Range("B24").Value = "max"
    Range("B25").Value = "avg"

    'Process each file
    lastDriveRow = Worksheets(myXGIGSheetName).Range("A:A").Cells.SpecialCells(xlCellTypeConstants).Count
    lastDiskSimRow = Worksheets(myDiskSimSheetName).Range("A:A").Cells.SpecialCells(xlCellTypeConstants).Count
    
    'Populate Drive and DiskSim Total Count cells
    Range("C2").Value = lastDriveRow - 1
    Range("D2").Value = lastDiskSimRow - 1
    Range("E2").Value = CalculatePercentage(Range("C2").Value, Range("D2").Value)
    If (lastDriveRow <> lastDiskSimRow) Then
        MsgBox ("Drive/DiskSim Miss-match:  Drive rows: " & lastDriveRow & Chr(13) & "DiskSim Rows: " & lastDiskSimRow & Chr(13) & "Max rows: " & maxDriveRows)
        CreateAndPopulateStatsWorkSheet = True
        Exit Function
    End If

    ' compare LBN fields
    For i = 2 To lastDriveRow
        temp1 = Worksheets(myDiskSimSheetName).Cells(i, 6).Value
        temp2 = Worksheets(myXGIGSheetName).Cells(i, 8).Value
        If (Worksheets(myDiskSimSheetName).Cells(i, 6).Value <> Worksheets(myXGIGSheetName).Cells(i, 8).Value) Then
            MsgBox ("Row Invalid: " & i & Chr(13) & temp1 & "<>" & temp2)
            CreateAndPopulateStatsWorkSheet = True
            Exit Function
        End If
    Next i

    ' Calculate Total Time
    temp1 = Worksheets(myXGIGSheetName).Cells(lastDriveRow, 2).Value
    temp2 = Worksheets(myXGIGSheetName).Cells(2, 1).Value
    Range("C3").Value = temp1 - temp2

    temp1 = Worksheets(myDiskSimSheetName).Cells(lastDriveRow, 2).Value
    Range("D3").Value = temp1 / 1000
    Range("E3").Value = CalculatePercentage(Range("C3").Value, Range("D3").Value)

    ' Calculate IOPS
    Range("C4").Value = Range("C2").Value / Range("C3").Value
    Range("D4").Value = Range("D2").Value / Range("D3").Value
    Range("E4").Value = CalculatePercentage(Range("C4").Value, Range("D4").Value)

    ' Calculate Total CCT
    Range("C5").Value = WorksheetFunction.Sum(Worksheets(myXGIGSheetName).Range("L2:L" & lastDriveRow))
    Range("D5").Value = Application.WorksheetFunction.Sum(Worksheets(myDiskSimSheetName).Range("AA2:AA" & lastDriveRow))
    Range("E5").Value = CalculatePercentage(Range("C5").Value, Range("D5").Value)

    ' Calculate Total qCCT
    Range("C6").Value = Application.WorksheetFunction.Sum(Worksheets(myXGIGSheetName).Range("M2:M" & lastDriveRow))
    Range("D6").Value = Application.WorksheetFunction.Sum(Worksheets(myDiskSimSheetName).Range("AB2:AB" & lastDriveRow))
    Range("E6").Value = CalculatePercentage(Range("C6").Value, Range("D6").Value)

    ' Calculate Stats: ToD
    Range("C7").Value = Application.WorksheetFunction.Min(Worksheets(myXGIGSheetName).Range("N2:N" & lastDriveRow))
    Range("C8").Value = Application.WorksheetFunction.Max(Worksheets(myXGIGSheetName).Range("N2:N" & lastDriveRow))
    Range("C9").Value = Application.WorksheetFunction.Average(Worksheets(myXGIGSheetName).Range("N2:N" & lastDriveRow))
    
    Range("D7").Value = Application.WorksheetFunction.Min(Worksheets(myDiskSimSheetName).Range("AB2:AB" & lastDriveRow))
    Range("D8").Value = Application.WorksheetFunction.Max(Worksheets(myDiskSimSheetName).Range("AB2:AB" & lastDriveRow))
    Range("D9").Value = Application.WorksheetFunction.Average(Worksheets(myDiskSimSheetName).Range("AB2:AB" & lastDriveRow))
    
    Range("E7").Value = CalculatePercentage(Range("C7").Value, Range("D7").Value)
    Range("E8").Value = CalculatePercentage(Range("C8").Value, Range("D8").Value)
    Range("E9").Value = CalculatePercentage(Range("C9").Value, Range("D9").Value)

    ' Calculate Stats: IO Driver Queue Depth
    Range("C10").Value = Application.WorksheetFunction.Min(Worksheets(myXGIGSheetName).Range("O2:O" & lastDriveRow))
    Range("C11").Value = Application.WorksheetFunction.Max(Worksheets(myXGIGSheetName).Range("O2:O" & lastDriveRow))
    Range("C12").Value = Application.WorksheetFunction.Average(Worksheets(myXGIGSheetName).Range("O2:O" & lastDriveRow))
    
    Range("D10").Value = Application.WorksheetFunction.Min(Worksheets(myDiskSimSheetName).Range("I2:I" & lastDriveRow))
    Range("D11").Value = Application.WorksheetFunction.Max(Worksheets(myDiskSimSheetName).Range("I2:I" & lastDriveRow))
    Range("D12").Value = Application.WorksheetFunction.Average(Worksheets(myDiskSimSheetName).Range("I2:I" & lastDriveRow))
    
    Range("E10").Value = CalculatePercentage(Range("C10").Value, Range("D10").Value)
    Range("E11").Value = CalculatePercentage(Range("C11").Value, Range("D11").Value)
    Range("E12").Value = CalculatePercentage(Range("C12").Value, Range("D12").Value)

    ' Calculate Stats: Disk Ctlr Queue Depth
    Range("C13").Value = Application.WorksheetFunction.Min(Worksheets(myXGIGSheetName).Range("O2:O" & lastDriveRow))
    Range("C14").Value = Application.WorksheetFunction.Max(Worksheets(myXGIGSheetName).Range("O2:O" & lastDriveRow))
    Range("C15").Value = Application.WorksheetFunction.Average(Worksheets(myXGIGSheetName).Range("O2:O" & lastDriveRow))
    
    Range("D13").Value = Application.WorksheetFunction.Min(Worksheets(myDiskSimSheetName).Range("S2:S" & lastDriveRow))
    Range("D14").Value = Application.WorksheetFunction.Max(Worksheets(myDiskSimSheetName).Range("S2:S" & lastDriveRow))
    Range("D15").Value = Application.WorksheetFunction.Average(Worksheets(myDiskSimSheetName).Range("S2:S" & lastDriveRow))
    
    Range("E13").Value = CalculatePercentage(Range("C13").Value, Range("D13").Value)
    Range("E14").Value = CalculatePercentage(Range("C14").Value, Range("D14").Value)
    Range("E15").Value = CalculatePercentage(Range("C15").Value, Range("D15").Value)

    ' Calculate Cache Hits
    Range("C16").Value = Application.WorksheetFunction.CountIf(Worksheets(myXGIGSheetName).Range("V2:V" & lastDriveRow), "hit")
    Range("D16").Value = Application.WorksheetFunction.CountIf(Worksheets(myDiskSimSheetName).Range("U2:U" & lastDriveRow), "BUFFER_WHOLE")
    Range("E16").Value = CalculatePercentage(Range("C16").Value, Range("D16").Value)

    Range("D17").Value = Application.WorksheetFunction.CountIf(Worksheets(myDiskSimSheetName).Range("U2:U" & lastDriveRow), "BUFFER_NOMATCH")
    Range("D18").Value = Application.WorksheetFunction.CountIf(Worksheets(myDiskSimSheetName).Range("U2:U" & lastDriveRow), "BUFFER_WHOLE")
    Range("D19").Value = Application.WorksheetFunction.CountIf(Worksheets(myDiskSimSheetName).Range("U2:U" & lastDriveRow), "BUFFER_PARTIAL")

    ' Calculate Stats: CCT
    Range("C20").Value = Application.WorksheetFunction.Min(Worksheets(myXGIGSheetName).Range("L2:L" & lastDriveRow))
    Range("C21").Value = Application.WorksheetFunction.Max(Worksheets(myXGIGSheetName).Range("L2:L" & lastDriveRow))
    Range("C22").Value = Application.WorksheetFunction.Average(Worksheets(myXGIGSheetName).Range("L2:L" & lastDriveRow))
    
    Range("D20").Value = Application.WorksheetFunction.Min(Worksheets(myDiskSimSheetName).Range("AA2:AA" & lastDriveRow))
    Range("D21").Value = Application.WorksheetFunction.Max(Worksheets(myDiskSimSheetName).Range("AA2:AA" & lastDriveRow))
    Range("D22").Value = Application.WorksheetFunction.Average(Worksheets(myDiskSimSheetName).Range("AA2:AA" & lastDriveRow))
    
    Range("E20").Value = CalculatePercentage(Range("C20").Value, Range("D20").Value)
    Range("E21").Value = CalculatePercentage(Range("C21").Value, Range("D21").Value)
    Range("E22").Value = CalculatePercentage(Range("C22").Value, Range("D22").Value)

    ' Calculate Stats: qCCT
    Range("C23").Value = Application.WorksheetFunction.Min(Worksheets(myXGIGSheetName).Range("M2:M" & lastDriveRow))
    Range("C24").Value = Application.WorksheetFunction.Max(Worksheets(myXGIGSheetName).Range("M2:M" & lastDriveRow))
    Range("C25").Value = Application.WorksheetFunction.Average(Worksheets(myXGIGSheetName).Range("M2:M" & lastDriveRow))
    
    Range("D23").Value = Application.WorksheetFunction.Min(Worksheets(myDiskSimSheetName).Range("AB2:AB" & lastDriveRow))
    Range("D24").Value = Application.WorksheetFunction.Max(Worksheets(myDiskSimSheetName).Range("AB2:AB" & lastDriveRow))
    Range("D25").Value = Application.WorksheetFunction.Average(Worksheets(myDiskSimSheetName).Range("AB2:AB" & lastDriveRow))
    
    Range("E23").Value = CalculatePercentage(Range("C23").Value, Range("D23").Value)
    Range("E24").Value = CalculatePercentage(Range("C24").Value, Range("D24").Value)
    Range("E25").Value = CalculatePercentage(Range("C25").Value, Range("D25").Value)

    temp3 = Range("C4").Value / 5
    Range("B1").Value = WorksheetFunction.Ceiling(temp3, 5)

    CreateAndPopulateStatsWorkSheet = False
End Function
Function CalculatePercentage(value1 As Variant, value2 As Variant) As Variant
    CalculatePercentage = 0
    If (0 <> value1) Then
        CalculatePercentage = (value1 - value2) / value1
    End If
End Function
Sub ImportCsvFile(FileName As Variant, position As Range)
  With ActiveSheet.QueryTables.Add(Connection:= _
      "TEXT;" & FileName _
      , Destination:=position)
      .Name = Replace(FileName, ".csv", "")
      .FieldNames = True
      .RowNumbers = False
      .FillAdjacentFormulas = False
      .RefreshOnFileOpen = False
      .BackgroundQuery = True
      .RefreshStyle = xlInsertDeleteCells
      .SavePassword = False
      .SaveData = True
      .AdjustColumnWidth = True
      .TextFilePromptOnRefresh = False
      .TextFilePlatform = xlMacintosh
      .TextFileStartRow = 1
      .TextFileParseType = xlDelimited
      .TextFileTextQualifier = xlTextQualifierDoubleQuote
      .TextFileConsecutiveDelimiter = False
      .TextFileTabDelimiter = True
      .TextFileSemicolonDelimiter = False
      .TextFileCommaDelimiter = False
      .TextFileSpaceDelimiter = False
      .TextFileOtherDelimiter = ","
      .TextFileColumnDataTypes = Array(1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
      .Refresh BackgroundQuery:=False
  End With
End Sub
Private Function GetUpper(varArray As Variant) As Integer
    Dim Upper As Integer
    On Error Resume Next
    Upper = UBound(varArray)
    If Err.Number Then
        If Err.Number = 9 Then
            Upper = 0
        Else
            With Err
                MsgBox "Error:" & .Number & "-" & .Description
            End With
            Exit Function
        End If
    Else
        Upper = UBound(varArray) + 1
    End If
    On Error GoTo 0
    GetUpper = Upper
End Function
Function CopyStatsToChartsXlsx(ByVal myChartFileName As String, ByVal mySpreadSheetFileNames As Variant, ByVal myNumFiles As Integer, ByVal myStatsSheetName As String) As Boolean
    Dim currSpreadSheetName As String
    Dim valuesBSU() As Variant
    Dim valuesIOPS() As Variant
    Dim myStatus As Boolean

    ReDim valuesIOPS(myNumFiles - 1)
    ReDim valuesBSU(myNumFiles - 1)
    
    ' Create new charts workbook and save it
    Workbooks.Add xlWBATWorksheet
    Application.DisplayAlerts = False
    ActiveWorkbook.SaveAs FileName:=myChartFileName
    Application.DisplayAlerts = True

    For fileIndex = 0 To myNumFiles - 1
        'get the current filename we are processing an create a worksheet in Charts with that name
        currSpreadSheetName = ExtractFileName(mySpreadSheetFileNames(fileIndex))
        Worksheets.Add after:=Worksheets(Worksheets.Count)
        ActiveSheet.Name = currSpreadSheetName
        
        Workbooks.Open FileName:=mySpreadSheetFileNames(fileIndex), ReadOnly:=True
        Workbooks(myChartFileName).Worksheets(currSpreadSheetName).Range("A1:E25").Value = Workbooks(mySpreadSheetFileNames(fileIndex)).Worksheets(myStatsSheetName).Range("A1:E25").Value
        Workbooks(mySpreadSheetFileNames(fileIndex)).Close
    Next
    ActiveWorkbook.Save
    ActiveWorkbook.Close
End Function
Function PlotDiskSimIOPS(ByVal mySpreadSheetFileNames As Variant, ByVal myNumFiles As Integer, ByVal myStatsSheetName As String) As Boolean
    Dim valuesBSU() As Variant
    Dim valuesIOPS() As Variant

    ReDim valuesIOPS(myNumFiles - 1)
    ReDim valuesBSU(myNumFiles - 1)
    For fileIndex = 0 To myNumFiles - 1
        Workbooks.Open FileName:=mySpreadSheetFileNames(fileIndex), ReadOnly:=True
        Workbooks(mySpreadSheetFileNames(fileIndex)).Worksheets(myStatsSheetName).Activate
        valuesBSU(fileIndex) = Range("B1").Value
        valuesIOPS(fileIndex) = Range("D4").Value
        Workbooks(mySpreadSheetFileNames(fileIndex)).Close
    Next
End Function
'true = error
Function GetDiskSimFilesToProcess(ByRef myDiskSimFilesName As Variant, ByRef myCurrentDirectory As String, ByRef myXGIGSheetName As String, ByRef myDiskSimSheetName As String, ByRef myXGIGFileNames() As String, ByRef myDiskSimFileNames() As String) As Boolean
    Dim lastRow As Integer

    ' get the filenames to process
    myDiskSimFilesName = Application.GetOpenFilename("Text Files (*.csv), *.csv")
    If (False = myDiskSimFilesName) Then
        GetDiskSimFilesToProcess = True
        Exit Function
    End If
    
    Application.Workbooks.OpenText FileName:=myDiskSimFilesName, _
                                   StartRow:=1, _
                                   DataType:=Excel.XlTextParsingType.xlDelimited, _
                                   TextQualifier:=Excel.XlTextQualifier.xlTextQualifierNone, _
                                   Comma:=True
    myCurrentDirectory = Range("A1").Value
    myXGIGSheetName = Range("A2").Value
    myDiskSimSheetName = Range("B2").Value
    
    ' populate spreadsheet names
    lastFileRow = Worksheets("DiskSimFiles").Range("A:A").Cells.SpecialCells(xlCellTypeConstants).Count
    ReDim myDiskSimFileNames(lastFileRow - 3)
    ReDim myXGIGFileNames(lastFileRow - 3)

    For fileRow = 3 To lastFileRow
        myXGIGFileNames(fileRow - 3) = Cells(fileRow, 1)
        myDiskSimFileNames(fileRow - 3) = Cells(fileRow, 2)
    Next fileRow

    'Discard workbook
    Application.DisplayAlerts = False
    ActiveWorkbook.Close
    Application.DisplayAlerts = True
    GetDiskSimFilesToProcess = False
End Function
Function DeleteAllWorkSheets() As Boolean
    Application.DisplayAlerts = False

    For Index = 1 To Worksheets.Count
        If ("Sheet1" <> Worksheets(Index).Name) Then
            Worksheets(Index).Delete
        End If
    Next

    Application.DisplayAlerts = True
    DeleteAllWorkSheets = False
End Function
Function ExtractFileName(ByVal myFileName As String) As String
    Dim parsedFileName() As String
    
    parsedFileName = Split(myFileName, ".")
    ExtractFileName = Mid(parsedFileName(0), InStrRev(parsedFileName(0), "\") + 1)
End Function
Function CreateChartData(ByVal myChartFileName As String, ByVal myXGIGChartSheetName As String, ByVal myDiskSimChartSheetName As String, ByVal mySpreadSheetFileNames As Variant, ByVal myNumFiles As Integer) As Boolean
    Dim currWorksheetName As String
    Dim myChartDataErrorSheetName
    Dim BSUindex As Integer
    Dim myColumn As Integer

    myChartDataErrorSheetName = "ChartData_Error"

    'Create new worksheets to hold the data
    Workbooks.Open FileName:=myChartFileName

    Worksheets.Add after:=Worksheets(1)
    ActiveSheet.Name = myXGIGChartSheetName
    
    Worksheets.Add after:=Worksheets(myXGIGChartSheetName)
    ActiveSheet.Name = myDiskSimChartSheetName
    
    Worksheets.Add after:=Worksheets(myDiskSimChartSheetName)
    ActiveSheet.Name = myChartDataErrorSheetName

    'start populating the new chart data worksheets
    'copy the data labels
    currWorksheetName = ExtractFileName(mySpreadSheetFileNames(0))
    Worksheets(myXGIGChartSheetName).Range("A1").Value = Worksheets(currWorksheetName).Range("A1").Value
    Worksheets(myXGIGChartSheetName).Range("A2:B25").Value = Worksheets(currWorksheetName).Range("A2:B25").Value
    
    Worksheets(myDiskSimChartSheetName).Range("A1").Value = Worksheets(currWorksheetName).Range("A1").Value
    Worksheets(myDiskSimChartSheetName).Range("A2:B25").Value = Worksheets(currWorksheetName).Range("A2:B25").Value
    
    Worksheets(myChartDataErrorSheetName).Range("A1").Value = Worksheets(currWorksheetName).Range("A1").Value
    Worksheets(myChartDataErrorSheetName).Range("A2:B25").Value = Worksheets(currWorksheetName).Range("A2:B25").Value

    'format columns
    Worksheets(myXGIGChartSheetName).Range("A:A").ColumnWidth = 19
    Worksheets(myXGIGChartSheetName).Range("A:A").Font.Bold = True
    Worksheets(myXGIGChartSheetName).Range("B:B").ColumnWidth = 17
    Worksheets(myXGIGChartSheetName).Range("B:B").Font.Bold = True

    Worksheets(myDiskSimChartSheetName).Range("A:A").ColumnWidth = 19
    Worksheets(myDiskSimChartSheetName).Range("A:A").Font.Bold = True
    Worksheets(myDiskSimChartSheetName).Range("B:B").ColumnWidth = 17
    Worksheets(myDiskSimChartSheetName).Range("B:B").Font.Bold = True

    Worksheets(myChartDataErrorSheetName).Range("A:A").ColumnWidth = 19
    Worksheets(myChartDataErrorSheetName).Range("A:A").Font.Bold = True
    Worksheets(myChartDataErrorSheetName).Range("B:B").ColumnWidth = 17
    Worksheets(myChartDataErrorSheetName).Range("B:B").Font.Bold = True

    BSUindex = 5
    myColumn = 3
    For myIndex = 0 To myNumFiles - 1
        currWorksheetName = ExtractFileName(mySpreadSheetFileNames(myIndex))
            
        Worksheets(myXGIGChartSheetName).Cells(1, myColumn).ColumnWidth = 16
        Worksheets(myXGIGChartSheetName).Cells(1, myColumn).Font.Bold = True
        Worksheets(myXGIGChartSheetName).Cells(1, myColumn).Value = BSUindex

        Worksheets(myDiskSimChartSheetName).Cells(1, myColumn).ColumnWidth = 16
        Worksheets(myDiskSimChartSheetName).Cells(1, myColumn).Font.Bold = True
        Worksheets(myDiskSimChartSheetName).Cells(1, myColumn).Value = BSUindex

        Worksheets(myChartDataErrorSheetName).Cells(1, myColumn).ColumnWidth = 16
        Worksheets(myChartDataErrorSheetName).Cells(1, myColumn).Font.Bold = True
        Worksheets(myChartDataErrorSheetName).Cells(1, myColumn).Value = BSUindex
        
        BSUindex = BSUindex + 5

        Worksheets(myXGIGChartSheetName).Activate
        Range(Cells(2, myColumn), Cells(25, myColumn)).Value = Worksheets(currWorksheetName).Range("C2:C25").Value
        Worksheets(myDiskSimChartSheetName).Activate
        Range(Cells(2, myColumn), Cells(25, myColumn)).Value = Worksheets(currWorksheetName).Range("D2:D25").Value
        Worksheets(myChartDataErrorSheetName).Activate
        Range(Cells(2, myColumn), Cells(25, myColumn)).Value = Worksheets(currWorksheetName).Range("E2:E25").Value
        Range(Cells(2, myColumn), Cells(25, myColumn)).NumberFormat = "%0.00"
        myColumn = myColumn + 1
    Next
    Workbooks(myChartFileName).Save
    Workbooks(myChartFileName).Close
End Function
Function PlotChartData(ByVal myChartFileName As String, ByVal myXGIGChartSheetName As String, ByVal myDiskSimChartSheetName As String, ByVal mySpreadSheetFileNames As Variant, ByVal myNumFiles As Integer) As Boolean
    Dim myChartDataErrorSheetName
    Dim myChartErrorSheetName

    Dim myRow As Integer
    Dim myStartColumn As Integer
    Dim myEndColumn As Integer

    myChartDataErrorSheetName = "ChartData_Error"
    myChartErrorSheetName = "Chart_Error"

    myStartColumn = 3
    myEndColumn = myStartColumn + myNumFiles - 1

    'Create new worksheets to hold the data
    'xlXYScatterLines
'    Charts("Chart1").ChartWizard _
        Gallery:=xlXYScatterLines, _
        HasLegend:=True, CategoryTitle:="Year", ValueTitle:="Sales"
    Workbooks.Open FileName:=myChartFileName

    Charts.Add after:=Worksheets(myChartDataErrorSheetName)
    ActiveChart.Name = myChartErrorSheetName
    
    'create the %Error chart
    Charts(myChartErrorSheetName).ChartWizard _
        Gallery:=xlXYScatterLines, _
        Format:=1, _
        HasLegend:=True, _
        Title:="Generic_600GBDisk vs DiskSim %error vs BSU" & Chr(13) & "post read enabled (WCE)", _
        CategoryTitle:="BSU", _
        ValueTitle:="%Error"
    
    'clear the default series
    With ActiveChart
        Do Until .SeriesCollection.Count = 0
            .SeriesCollection(1).Delete
        Loop
    End With

    ' Total Requests
    Worksheets(myChartDataErrorSheetName).Activate

    myRow = 2
    With Charts(myChartErrorSheetName).SeriesCollection.NewSeries
        .Name = Worksheets(myChartDataErrorSheetName).Range("A2")
        .Values = Worksheets(myChartDataErrorSheetName).Range(Cells(myRow, myStartColumn), Cells(myRow, myEndColumn))
        .XValues = Worksheets(myChartDataErrorSheetName).Range(Cells(1, myStartColumn), Cells(1, myEndColumn))
    End With

    ' Total Time
    myRow = 3
    With Charts(myChartErrorSheetName).SeriesCollection.NewSeries
        .Name = Worksheets(myChartDataErrorSheetName).Range("A3")
        .Values = Worksheets(myChartDataErrorSheetName).Range(Cells(myRow, myStartColumn), Cells(myRow, myEndColumn))
        .XValues = Worksheets(myChartDataErrorSheetName).Range(Cells(1, myStartColumn), Cells(1, myEndColumn))
    End With

    ' IOPS
    myRow = 4
    With Charts(myChartErrorSheetName).SeriesCollection.NewSeries
        .Name = Worksheets(myChartDataErrorSheetName).Range("A4")
        .Values = Worksheets(myChartDataErrorSheetName).Range(Cells(myRow, myStartColumn), Cells(myRow, myEndColumn))
        .XValues = Worksheets(myChartDataErrorSheetName).Range(Cells(1, myStartColumn), Cells(1, myEndColumn))
    End With

    ' Total CCT(ms)
    myRow = 5
    With Charts(myChartErrorSheetName).SeriesCollection.NewSeries
        .Name = Worksheets(myChartDataErrorSheetName).Range("A5")
        .Values = Worksheets(myChartDataErrorSheetName).Range(Cells(myRow, myStartColumn), Cells(myRow, myEndColumn))
        .XValues = Worksheets(myChartDataErrorSheetName).Range(Cells(1, myStartColumn), Cells(1, myEndColumn))
    End With

    ' Total qCCT(ms)
    myRow = 6
    With Charts(myChartErrorSheetName).SeriesCollection.NewSeries
        .Name = Worksheets(myChartDataErrorSheetName).Range("A6")
        .Values = Worksheets(myChartDataErrorSheetName).Range(Cells(myRow, myStartColumn), Cells(myRow, myEndColumn))
        .XValues = Worksheets(myChartDataErrorSheetName).Range(Cells(1, myStartColumn), Cells(1, myEndColumn))
    End With
    
    Workbooks(myChartFileName).Save
    Workbooks(myChartFileName).Close
End Function
