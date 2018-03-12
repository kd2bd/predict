VERSION 5.00
Object = "{248DD890-BB45-11CF-9ABC-0080C7E7B78D}#1.0#0"; "MSWINSCK.OCX"
Begin VB.Form frmWinPredictClient 
   Caption         =   "WinPredictClient"
   ClientHeight    =   5175
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   6495
   LinkTopic       =   "Form1"
   ScaleHeight     =   5175
   ScaleWidth      =   6495
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdConnect 
      Caption         =   "Get List"
      Enabled         =   0   'False
      Height          =   375
      Left            =   4080
      TabIndex        =   7
      Top             =   120
      Width           =   855
   End
   Begin MSWinsockLib.Winsock wsckPredict 
      Left            =   1200
      Top             =   4200
      _ExtentX        =   741
      _ExtentY        =   741
      _Version        =   393216
      Protocol        =   1
   End
   Begin VB.Timer timRefresh 
      Enabled         =   0   'False
      Left            =   480
      Top             =   4200
   End
   Begin VB.TextBox txtAzEl 
      Height          =   2535
      Left            =   240
      MultiLine       =   -1  'True
      ScrollBars      =   2  'Vertical
      TabIndex        =   6
      Top             =   1560
      Visible         =   0   'False
      Width           =   6015
   End
   Begin VB.CommandButton cmdNextPass 
      Caption         =   "Next Pass"
      Enabled         =   0   'False
      Height          =   375
      Left            =   5040
      TabIndex        =   5
      Top             =   840
      Width           =   855
   End
   Begin VB.CommandButton cmdCurrent 
      Caption         =   "Current"
      Enabled         =   0   'False
      Height          =   375
      Left            =   4080
      TabIndex        =   4
      Top             =   840
      Width           =   855
   End
   Begin VB.ComboBox cmbSatList 
      Enabled         =   0   'False
      Height          =   315
      Left            =   1680
      TabIndex        =   2
      Top             =   840
      Width           =   1935
   End
   Begin VB.TextBox txtConnectTo 
      Height          =   375
      Left            =   1680
      TabIndex        =   1
      Text            =   "my-linux-box"
      Top             =   120
      Width           =   1935
   End
   Begin VB.Label lblSatList 
      Caption         =   "Select Satellite:"
      Height          =   375
      Left            =   360
      TabIndex        =   3
      Top             =   840
      Width           =   1215
   End
   Begin VB.Label lblConnectTo 
      Caption         =   "PREDICT host:"
      Height          =   255
      Left            =   360
      TabIndex        =   0
      Top             =   240
      Width           =   1095
   End
End
Attribute VB_Name = "frmWinPredictClient"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
' the following line must match whatever the predict server uses
Const PredictPort = 1210
Const GetSatList = "GET_LIST"
Const GetSatPos = "GET_SAT_POS" ' plus satellite name
Const GetNextPass = "PREDICT"   ' plus satellute name
Dim strWsckData As String
Dim bGettingList As Boolean
Dim bGettingNextPass As Boolean
Dim bGettingCurrent As Boolean
Dim strUnixDate As String
Dim strPredictHost As String


Private Sub cmbSatList_Click()
If cmbSatList.ListIndex >= 0 Then
   cmdCurrent.Enabled = True
   cmdNextPass.Enabled = True
End If
End Sub

Private Sub cmdConnect_Click()

strWsckData = vbLf
bGettingList = True
strUnixDate = ""
wsckPredict.SendData GetSatList

While Len(strWsckData) = 1
   DoEvents
Wend
LoadItems
cmdConnect.Enabled = False
bGettingList = False
End Sub

Private Sub cmdCurrent_Click()
   strUnixDate = ""
   If bGettingCurrent = False Then
      bGettingCurrent = True
      strWsckData = ""
      txtAzEl.Text = ""
      timRefresh.Interval = 1 'ms
      timRefresh.Enabled = True
      cmdCurrent.Caption = "Stop"
      cmdNextPass.Enabled = False
      cmbSatList.Enabled = False
   Else
      cmdCurrent.Caption = "Current"
      timRefresh.Enabled = False
      bGettingCurrent = False
      txtAzEl.Visible = False
      cmdNextPass.Enabled = True
      cmbSatList.Enabled = True
   End If
End Sub

Private Sub cmdNextPass_Click()
  If bGettingNextPass = False Then
     bGettingNextPass = True
     cmdCurrent.Enabled = False
     strWsckData = ""
     txtAzEl.Text = "Processing.......please wait"
     wsckPredict.SendData GetNextPass + " " + _
       Chr(34) + cmbSatList.Text + Chr(34) _
       + " " + strUnixDate + vbLf
     While Len(strWsckData) = 0 _
     And InStr(1, strWsckData, Chr(26)) < 1 ' wait for ctrl-z
       DoEvents
     Wend
     txtAzEl.Text = ""
     bGettingNextPass = False
     txtAzEl.Visible = True
     txtAzEl.Height = 2500
     DisplayResponse
     cmdCurrent.Enabled = True
     strWsckData = ""
  End If
End Sub

Private Sub Form_Load()
' if host is supplied on command line, we'll automatically connect
strPredictHost = Command
bGettingList = False
bGettingNextPass = False
bGettingCurrent = False
If Len(strPredictHost) > 0 Then
   txtConnectTo.Text = strPredictHost
   'cmdConnect_Click
   strWsckData = vbLf
   bGettingList = True
   strUnixDate = ""
   wsckPredict.SendData GetSatList

   While Len(strWsckData) = 1
      DoEvents
   Wend
   LoadItems
   cmdConnect.Enabled = False
   bGettingList = False
End If
End Sub

Private Sub timRefresh_Timer()
strWsckData = ""
txtAzEl.Text = ""
wsckPredict.SendData GetSatPos + " " + Chr(34) + cmbSatList.Text + Chr(34) + vbLf

While Len(strWsckData) = 0
   DoEvents
Wend
txtAzEl.Visible = True
txtAzEl.Height = 500
DisplayResponse
timRefresh.Interval = 1000 'ms
timRefresh.Enabled = True
End Sub

Private Sub txtConnectTo_Change()
If Len(txtConnectTo.Text) > 0 Then
   cmdConnect.Enabled = True
   wsckPredict.RemoteHost = txtConnectTo.Text
   wsckPredict.RemotePort = PredictPort
   wsckPredict.LocalPort = PredictPort
Else
   cmdConnect.Enabled = False
End If
End Sub

Private Sub wsckPredict_DataArrival(ByVal bytesTotal As Long)

Dim strData As String
    wsckPredict.GetData strData, vbString
    strWsckData = strWsckData & strData

End Sub
Private Sub DisplayResponse()
Dim iStrg As Integer
strUnixDate = ""
'bDoingDate = False
For iStrg = 1 To Len(strWsckData)
DoEvents
If Mid$(strWsckData, iStrg, 1) <> vbLf Then
   txtAzEl.Text = txtAzEl.Text & Mid$(strWsckData, iStrg, 1)
Else
   txtAzEl.Text = txtAzEl.Text & vbCrLf
   If Len(strWsckData) > iStrg + 11 _
   And Val(Mid$(strWsckData, iStrg + 1, 10)) > 0 Then
     strUnixDate = CStr(2 + Val(Mid$(strWsckData, iStrg + 1, 10)))
   End If
End If
Next iStrg
DoEvents
End Sub
Private Sub LoadItems()
Dim iPtr As Integer
Dim strItem As String
strItem = ""
For iPtr = 1 To Len(strWsckData)
   If Mid$(strWsckData, iPtr, 1) = vbLf Then
      If Len(strItem) > 0 Then
         cmbSatList.AddItem strItem
      End If
      strItem = ""
   Else
      strItem = strItem & Mid$(strWsckData, iPtr, 1)
   End If
Next iPtr
txtConnectTo.Enabled = False
cmbSatList.Text = "Select Satellite"
cmbSatList.Enabled = True
End Sub

