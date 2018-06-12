import wx
import serial
import time
import os
import serial.tools.list_ports

class COM_Port_Lister():
    def Run(self):
        class SecondFrame(wx.Dialog):
            def __init__(self, objMain):
                """Constructor"""
                wx.Dialog.__init__(self, None, title="Umrechnung von Absolutzeiten", size=(800, 700), pos=(600, 100))
                self.SetFont(wx.Font(9, wx.SWISS, wx.NORMAL, wx.NORMAL, False,'Liberation Sans'))                
                panel = wx.Panel(self)
                vbox2 = wx.GridSizer(21, 4, 0, 0)
                self.objMain = objMain
                self.update = False
                self.absframe = None
                self.absfields = dict()
                self.intfields = dict()
                self.is_triggered = dict()
                for ii in range(0, 16):
                    labelstr = "Absolute Startzeit Kanal " + str(ii+1) + ":"
                    lint = wx.StaticText(panel,  label = labelstr) 
                    vbox2.Add(lint, 1, wx.ALIGN_LEFT|wx.ALL,5) 
                    self.tabs = wx.TextCtrl(panel, name="Abstime%s"%(ii), value="")
                    vbox2.Add(self.tabs,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)
                    self.cb = wx.CheckBox(panel, label="Triggered?", style=0, name="IsTrig%s"%(ii))
                    if(ii == 0):
                        self.cb.SetValue(True)
                        self.cb.Disable()
                    else:
                        self.is_triggered[str(self.cb.GetName())] = self.cb
                    
                    vbox2.Add(self.cb,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)
                    
                    if(ii == 0):
                        self.tint = wx.StaticText(panel,  label="Resultierende Intervalle:")
                    else:
                        self.tint = wx.TextCtrl(panel, name="Int%s"%(ii), value="")
                        self.tint.Disable()
                        self.intfields[str(self.tint.GetName())] = self.tint
                    vbox2.Add(self.tint,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)
                    
                    self.absfields[str(self.tabs.GetName())] = self.tabs
                
                self.btn_takeover = wx.Button(panel, label="Werte aus Hauptfenster")
                self.btn_takeover.Bind(wx.EVT_BUTTON,self.on_takeover_clicked)
                self.btn_reset = wx.Button(panel, label="Felder leeren")
                self.btn_reset.Bind(wx.EVT_BUTTON,self.on_reset_clicked)
                self.btn_calcints = wx.Button(panel, label="Berechne Intervalle")
                self.btn_calcints.Bind(wx.EVT_BUTTON,self.on_calcints_clicked)               
                self.btn_setints = wx.Button(panel, label="Werte ins Hauptfenster")
                self.btn_setints.Bind(wx.EVT_BUTTON,self.on_setints_clicked)
                offsettxt = wx.StaticText(panel,  label = "Offset Kanal 1:")
                self.offsval = wx.TextCtrl(panel, name="offset", value="0:00.00")
                
                
                vbox2.Add(self.btn_takeover,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)
                vbox2.Add(self.btn_reset,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)
                vbox2.Add(self.btn_calcints,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)               
                vbox2.Add(self.btn_setints,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)    
                vbox2.Add(offsettxt,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5) 
                vbox2.Add(self.offsval,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)                 
                
                panel.SetSizer(vbox2)

                self.Show() 
                #self.Fit()

            def parse_to_float(self, tstamp):
                mins = 0
                secs = 0.0
                if len(tstamp) == 0:
                    return 65535
                tstamp = tstamp.replace(',', '.')
                try:
                    if(tstamp.count(':') == 1):
                        if(tstamp.find(':') != 0):
                            [mins, secs] = tstamp.split(':', 1)
                            mins = int(mins)
                            secs = float(secs)
                        else:
                            tstamp = tstamp.replace(':', '0')
                            secs = float(tstamp)
                    if(tstamp.count(':') == 0):
                        secs = float(tstamp)
                        
                    if(tstamp.count(':') > 1):
                        return 65535
                except ValueError:
                    pass
                
                return (mins * 6000 + int((secs + 0.005)*100))
            
            def parse_to_timestring(self, hunval):
                try:
                    hunval = int(hunval)                
                    if(hunval < 0):
                        hunval = 0
                except ValueError:
                    return "FEHLER"
                
                mins = 0
                secs = 0.0
                
                mins = int(hunval/6000)
                secs = (hunval - mins*6000.0)/100.0
                
                return("%02d:%05.2f"%(mins,secs))
                
            def on_reset_clicked(self, event):
                for ii in range(0, 16):
                    self.absfields["Abstime%s"%(ii)].SetValue("")
                    if ii > 0:
                        self.is_triggered["IsTrig%s"%(ii)].SetValue(False)
                        self.intfields["Int%s"%(ii)].SetValue("")
                
            def on_calcints_clicked(self, event):
                for ii in range(0, 15):
                    if self.is_triggered["IsTrig%s"%(ii+1)].GetValue():
                        self.intfields["Int%s"%(ii+1)].SetValue( "Trigger" )
                    else:
                        tempvar1 = self.parse_to_float(self.absfields["Abstime%s"%(ii+1)].GetValue())
                        tempvar2 = self.parse_to_float(self.absfields["Abstime%s"%(ii)].GetValue())
                        if(tempvar1 != 65535 and tempvar2 != 65535 and tempvar1 >= tempvar2 and tempvar1-tempvar2 < 60000):
                            self.intfields["Int%s"%(ii+1)].SetValue( self.parse_to_timestring(tempvar1 - tempvar2) )
                        else:
                            self.intfields["Int%s"%(ii+1)].SetValue( "" )
            
            def on_setints_clicked(self, event):
                self.update = True
                if self.objMain.t3.GetValue() != "S":
                    if self.objMain.t2.GetSelection() == 1:
                        for x in self.intfields:
                            if len(self.intfields[x].GetValue()) > 0:
                                self.objMain.intfields[x].SetValue(self.intfields[x].GetValue())
                    else:
                        if len(self.intfields["Int1"].GetValue()) > 0:
                            self.objMain.intfields["Int1"].SetValue(self.intfields["Int1"].GetValue())
                            
            def on_takeover_clicked(self, event):
                self.absfields["Abstime0"].SetValue( self.parse_to_timestring(self.parse_to_float(self.offsval.GetValue())) )
                for x in range(1,16):
                    if(self.objMain.intfields["Int%s"%(x)].GetValue().lower() in ["trigger", "t", "tr", "tri", "trig", "trigg", "trigge", "triggere", "triggered"]):
                        self.absfields["Abstime%s"%(x)].SetValue(self.parse_to_timestring( 0 + self.parse_to_float(self.absfields["Abstime%s"%(x-1)].GetValue()) ))
                        self.is_triggered["IsTrig%s"%(x)].SetValue(True)
                    else:
                        self.absfields["Abstime%s"%(x)].SetValue(self.parse_to_timestring( self.parse_to_float(self.objMain.intfields["Int%s"%(x)].GetValue()) + self.parse_to_float(self.absfields["Abstime%s"%(x-1)].GetValue()) ))
            
        class displayDialog(wx.Dialog):
            def __init__(self, parent):
                wx.Dialog.__init__(self, parent, title="FUOCO STEP - Einstellungen", size=(400, 700), pos=(200,100))#
                self.SetFont(wx.Font(9, wx.SWISS, wx.NORMAL, wx.NORMAL, False,'Liberation Sans'))
                self.panel = wx.Panel(self) 
                vbox = wx.GridSizer(21, 2, 0, 0)
                
                self.show_abstimes = False
                self.absframe = None
                
                l1 = wx.StaticText(self.panel,  label = "COM-Port:") 
                vbox.Add(l1, 1, wx.ALIGN_RIGHT|wx.ALL,5) 
                ComPortList = [x[0] for x in list(serial.tools.list_ports.comports())]
                self.t1 = wx.ComboBox(self.panel, choices=ComPortList)
                self.t1.SetSelection(0)                                
                vbox.Add(self.t1,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)             
                
                l3 = wx.StaticText(self.panel, label = "Schalterposition:") 
                vbox.Add(l3, 1, wx.ALIGN_RIGHT|wx.ALL,5) 
                self.t3 = wx.TextCtrl(self.panel,  style=wx.TE_READONLY, value="")
                vbox.Add(self.t3,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)  
                
                l2 = wx.StaticText(self.panel,  label = "Stepper-Modus:") 
                vbox.Add(l2, 1, wx.ALIGN_RIGHT|wx.ALL,5) 
                self.t2 = wx.ComboBox(self.panel, choices=['Fest', 'Variabel'])
                self.t2.SetSelection(0)                                
                vbox.Add(self.t2,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5)
                self.t2.Disable()                
                
                self.intfields = dict()
                self.intfields_storage = ["Trigger"]
                for ii in range(1, 16):
                    if(ii == 1):
                        labelstr = "Intervall " + str(ii) + "/Festintervall:"
                    else:
                        labelstr = "Intervall " + str(ii) + ":"
                    lint = wx.StaticText(self.panel,  label = labelstr) 
                    vbox.Add(lint, 1, wx.ALIGN_RIGHT|wx.ALL,5) 
                    self.tint = wx.TextCtrl(self.panel, name="Int%s"%(ii), value="00:00.00")
                    self.tint.Disable()
                    vbox.Add(self.tint,1,wx.EXPAND|wx.ALIGN_LEFT|wx.ALL,5) 
                    self.intfields[str(self.tint.GetName())] = self.tint
                    self.intfields_storage.append(str(self.intfields[str(self.tint.GetName())].GetValue()))
                    
                
                self.btn_mode_write = wx.Button(self.panel, label="Modus setzen")
                self.btn_mode_write.Bind(wx.EVT_BUTTON,self.on_mode_write_clicked)
                vbox.Add(self.btn_mode_write,0, wx.EXPAND)
                self.btn_mode_write.Disable()
                
                self.btn_intervals_write = wx.Button(self.panel, label="Intervall(e) setzen")
                self.btn_intervals_write.Bind(wx.EVT_BUTTON,self.on_intervals_write_clicked)
                vbox.Add(self.btn_intervals_write,0, wx.EXPAND)
                self.btn_intervals_write.Disable()                
                
                self.btn_read = wx.Button(self.panel, label="Einstellungen auslesen")
                self.btn_read.Bind(wx.EVT_BUTTON,self.on_read_clicked)
                vbox.Add(self.btn_read,0, wx.EXPAND)
                
                self.btn_default = wx.Button(self.panel, label="Standardwerte widerherstellen")
                self.btn_default.Bind(wx.EVT_BUTTON,self.on_default_clicked)
                vbox.Add(self.btn_default,0, wx.EXPAND) 
                
                self.btn_cancel = wx.Button(self.panel, label="Beenden")
                self.btn_cancel.Bind(wx.EVT_BUTTON,self.on_cancel_clicked)
                vbox.Add(self.btn_cancel, 0, wx.EXPAND)

                self.btn_abstimes = wx.Button(self.panel, label="Absolutzeiten <<" if self.show_abstimes else "Absolutzeiten >>")
                self.btn_abstimes.Bind(wx.EVT_BUTTON,self.on_abstimes_clicked)
                vbox.Add(self.btn_abstimes,0,wx.EXPAND)

                self.panel.SetSizer(vbox)
                
                self.Show() 
                
                #self.Fit()

                
            
            def on_mode_write_clicked(self, event):
                self.perform_changes = True
                if (self.t1.GetString(self.t1.GetSelection()) != ""):
                    # print("Verbinde mit " + self.t1.GetString(self.t1.GetSelection()))
                    
                    ser = serial.Serial(self.t1.GetString(self.t1.GetSelection()), 9600, timeout=.2)
                    self.serialtransfer(ser, '\redit\r')
                    self.serialtransfer(ser, 'm')
                    stmode = str(self.t2.GetString(self.t2.GetSelection())[0])
                    self.serialtransfer(ser, stmode)
                    
                    ser.close()
                    
                    if(stmode == "V"):
                        for ii in range(2, 16):
                            self.intfields["Int%s"%(ii)].Enable()
                            
                    if(stmode == "F"):
                        for ii in range(2, 16):
                            self.intfields["Int%s"%(ii)].Disable()
                            
                    self.on_read_clicked(None)
            
            def on_intervals_write_clicked(self, event):
                self.perform_changes = True
                if (self.t1.GetString(self.t1.GetSelection()) != ""):                 
                    self.btn_intervals_write.Disable()
                    self.btn_read.Disable()
                    self.btn_default.Disable()
                    self.btn_cancel.Disable()
                    self.btn_mode_write.Disable()
                    
                    print("Setting intervals")
                    print("Opening serial port")
                    ser = serial.Serial(self.t1.GetString(self.t1.GetSelection()), 9600, timeout=.2)
                    
                    for ii in range(1, 16):                       
                        # if(self.intfields_storage[ii] != self.intfields["Int%s"%(ii)].GetValue()):
                        print(("Setting interval %s..."%(str(ii))))
                        ser.write('\redit\r')
                        test = self.serialtransfer(ser, 'i')
                        print((' '.join(test)))
                        
                        # Case for fixed interval
                        if (ii == 1):
                            if ("Neuer Wert" in ' '.join(test)):
                                ival = str(self.intfields["Int%s"%(ii)].GetValue())
                                if ival.lower() in ["trigger", "t", "tr", "tri", "trig", "trigg", "trigge", "triggere", "triggered"]:
                                    ival = "10:00.00"
                                self.serialtransfer(ser, (ival + '\r'))
                                self.serialtransfer(ser, str(str(ii) + '\r'))
                                break   
                            if ("Schalterstellung S" in  ' '.join(test)):
                                self.serialtransfer(ser, '\r')
                                break
                        
                        # Case for variable interval
                        self.serialtransfer(ser, str(str(ii) + '\r'))
                        ival = str(self.intfields["Int%s"%(ii)].GetValue())
                        if ival.lower() in ["trigger", "t", "tr", "tri", "trig", "trigg", "trigge", "triggere", "triggered"]:
                            ival = "10:00.00"
                        self.serialtransfer(ser, (ival + '\r'))
                        self.serialtransfer(ser, '\r')
                    
                    print("Closing serial port")
                    ser.close()
                    
                    self.btn_mode_write.Enable()
                    self.btn_intervals_write.Enable()
                    self.btn_read.Enable()
                    self.btn_default.Enable()
                    self.btn_cancel.Enable()
                    
                    self.on_read_clicked(None)

            def on_read_clicked(self, event):
                self.perform_changes = True
                if (self.t1.GetString(self.t1.GetSelection()) != ""):
                    print("Read current settings")
                    print("Open serial port")
                    ser = serial.Serial(self.t1.GetString(self.t1.GetSelection()), 9600, timeout=.2)
                    print("Get data")
                    ser.write('\redit\r')
                    num_of_lines = 2
                    
                    for i in range(5):
                        x = (ser.readline())
                        if "Schalterstellung " in x:
                            swpos = str(x.split("Schalterstellung ", 1)[1].replace(":",""))
                            self.t3.SetValue(swpos)
                        if " Intervall" in x:
                            num_of_lines = 2 if "Fest" in x else 19
                    
                    read_lines = []
                    for i in range(num_of_lines):
                        read_lines.append(ser.readline())
                    
                    self.serialtransfer(ser, 'x')
                    self.serialtransfer(ser, '\rcls\r')
                    
                    print("Processing data")
                    if num_of_lines == 2:
                        tval = x.split(': ', 1)[1]
                        self.t2.SetSelection(0)
                        self.intfields["Int1"].SetValue(tval)
                        self.intfields["Int1"].Enable()
                        
                        for ii in range(2, 16):
                            self.intfields["Int%s"%(ii)].Disable()
                       
                    else:
                        self.t2.SetSelection(1)
                        fieldctr = 1
                        for ii in read_lines:
                            if (": " in ii) and (" 0 (Start)" not in ii) and (fieldctr < 16):
                                tval = ii.split(': ', 1)[1].split('\r', 1)[0]
                                self.intfields["Int%s"%(fieldctr)].SetValue(tval)
                                self.intfields["Int%s"%(fieldctr)].Enable()
                                self.intfields_storage[fieldctr] = tval
                                fieldctr += 1
                   
                    print("Closing serial port")
                    ser.close()
                    self.btn_mode_write.Enable()
                    self.btn_intervals_write.Enable()
                    self.t2.Enable()
                    
            def on_default_clicked(self, event):
                ser = serial.Serial(self.t1.GetString(self.t1.GetSelection()), 9600, timeout=.2)
                self.serialtransfer(ser, '\rdefault\r')
                ser.close()
                
            def on_abstimes_clicked(self, event):
                self.show_abstimes = not self.show_abstimes
                
                if(self.show_abstimes):
                    if(bool(self.absframe)):
                        self.absframe.Show()
                    else:
                        self.absframe = SecondFrame(self)  
                    self.btn_abstimes.SetLabel("Absolutzeiten <<")
                else:
                    if(bool(self.absframe)):
                        self.absframe.Hide()
                    self.btn_abstimes.SetLabel("Absolutzeiten >>")
                    
            
            def serialtransfer(self, ser, stri):
                ticktick = time.time()
                answ = ""
                for s in stri:
                    ser.write(s)

                waits = 10000
                while(ser.inWaiting() != waits):
                    waits = ser.inWaiting()
                    time.sleep(.005)
                                                   
                for i in range(waits):
                    answ += ser.read()
                
                tocktock = time.time() - ticktick
                return answ.splitlines()
                   
                
            def on_cancel_clicked(self, event):
                self.perform_changes = False
                if(bool(self.absframe)):
                    self.absframe.Destroy()
                self.Destroy()              
        
        app = wx.App(False)
        frame = displayDialog(None)
        font = wx.Font(28, wx.DEFAULT, wx.NORMAL, wx.NORMAL, False, "FreeSerif")
        frame.SetFont(font)
        frame.ShowModal()
    
if __name__ == "__main__":
    COM_Port_Lister().Run()
