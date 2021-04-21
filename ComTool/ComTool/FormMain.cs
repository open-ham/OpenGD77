/*
 * Copyright (C)2019 Kai Ludwig, DG4KLU
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

using System;
using System.ComponentModel;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Threading;
using System.Windows.Forms;

namespace ComTool
{
    public partial class FormMain : Form
    {
        bool running = false;
        BackgroundWorker worker;
        bool stop_worker = false;
        bool form_close = false;

        SerialPort port = null;

        StreamWriter writer;

        SaveFileDialog saveFileDialog = new SaveFileDialog();
        OpenFileDialog openFileDialog = new OpenFileDialog();
        int data_start = 0;
        int data_length = 0;
        int data_pos = 0;
        int data_mode = 0;
        int data_sector = 0;
        Stream fileStream;
        int old_progress = 0;

        public FormMain()
        {
            InitializeComponent();
        }

        private void FormMain_Load(object sender, EventArgs e)
        {
            this.Location = new Point((Screen.PrimaryScreen.WorkingArea.Width - this.Width) / 2, (Screen.PrimaryScreen.WorkingArea.Height - this.Height) / 2);

            loadCOMPortlist();

            saveFileDialog.InitialDirectory = Directory.GetCurrentDirectory();
            saveFileDialog.Filter = "bin files (*.bin)|*.bin|All files (*.*)|*.*";
            saveFileDialog.FilterIndex = 1;
            saveFileDialog.RestoreDirectory = true;

            openFileDialog.InitialDirectory = Directory.GetCurrentDirectory();
            openFileDialog.Filter = "bin files (*.bin)|*.bin|All files (*.*)|*.*";
            openFileDialog.FilterIndex = 1;
            openFileDialog.RestoreDirectory = true;
        }

        private void FormMain_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (running)
            {
                stop_worker = true;
                form_close = true;
                e.Cancel = true;
            }
        }

        void loadCOMPortlist()
        {
            string old_item = comboBoxCOMPorts.Text;
            comboBoxCOMPorts.Items.Clear();
            string[] ports = SerialPort.GetPortNames();
            foreach (string port in ports)
            {
                comboBoxCOMPorts.Items.Add(port);
            }
            if (comboBoxCOMPorts.Items.Contains(old_item))
            {
                comboBoxCOMPorts.Text = old_item;
            }
        }

        void UpdateStatus(string text)
        {
            if (labelStatusText.InvokeRequired)
                labelStatusText.Invoke(new MethodInvoker(delegate ()
                {
                    labelStatusText.Text = text;
                }));
            else
            {
                labelStatusText.Text = text;
            }
        }

        private void ClearLog()
        {
            richTextBoxLog.BeginInvoke(new Action(() =>
            {
                richTextBoxLog.Text = "";
            }));
        }

        private void SetLog(String text)
        {
            richTextBoxLog.BeginInvoke(new Action(() =>
            {
                if (richTextBoxLog.Text != "")
                {
                    richTextBoxLog.AppendText("\r\n");
                }
                richTextBoxLog.AppendText(text);
                if (richTextBoxLog.Text.Length > 32000)
                {
                    richTextBoxLog.Text = richTextBoxLog.Text.Substring(richTextBoxLog.Text.Length - 32000);
                }
                richTextBoxLog.SelectionStart = richTextBoxLog.Text.Length;
                richTextBoxLog.ScrollToCaret();
                if (checkBoxLogToFile.Checked)
                {
                    writer.WriteLine(text);
                    writer.Flush();
                }
            }));
        }

        bool prepare_sector(int address, ref byte[] sendbuffer, ref byte[] readbuffer)
        {
            data_sector = address / 4096;

            sendbuffer[0] = (byte)'W';
            sendbuffer[1] = 1;
            sendbuffer[2] = (byte)((data_sector >> 16) & 0xFF);
            sendbuffer[3] = (byte)((data_sector >> 8) & 0xFF);
            sendbuffer[4] = (byte)((data_sector >> 0) & 0xFF);
            port.Write(sendbuffer, 0, 5);
            while (port.BytesToRead == 0)
            {
                Thread.Sleep(0);
            }
            port.Read(readbuffer, 0, 64);

            return ((readbuffer[0] == sendbuffer[0]) && (readbuffer[1] == sendbuffer[1]));
        }

        bool send_data(int address, int len, ref byte[] sendbuffer, ref byte[] readbuffer)
        {
            sendbuffer[0] = (byte)'W';
            sendbuffer[1] = 2;
            sendbuffer[2] = (byte)((address >> 24) & 0xFF);
            sendbuffer[3] = (byte)((address >> 16) & 0xFF);
            sendbuffer[4] = (byte)((address >> 8) & 0xFF);
            sendbuffer[5] = (byte)((address >> 0) & 0xFF);
            sendbuffer[6] = (byte)((len >> 8) & 0xFF);
            sendbuffer[7] = (byte)((len >> 0) & 0xFF);
            port.Write(sendbuffer, 0, len + 8);
            while (port.BytesToRead == 0)
            {
                Thread.Sleep(0);
            }
            port.Read(readbuffer, 0, 64);

            return ((readbuffer[0] == sendbuffer[0]) && (readbuffer[1] == sendbuffer[1]));
        }

        bool write_sector(ref byte[] sendbuffer, ref byte[] readbuffer)
        {
            data_sector = -1;

            sendbuffer[0] = (byte)'W';
            sendbuffer[1] = 3;
            port.Write(sendbuffer, 0, 2);
            while (port.BytesToRead == 0)
            {
                Thread.Sleep(0);
            }
            port.Read(readbuffer, 0, 64);

            return ((readbuffer[0] == sendbuffer[0]) && (readbuffer[1] == sendbuffer[1]));
        }

        void worker_DoWork(object sender, DoWorkEventArgs e)
        {
            byte[] sendbuffer = new byte[512];
            byte[] readbuffer = new byte[512];

            byte[] com_Buf = new byte[256];
            int com_Buf_pos = 0;

            stop_worker = false;

            ClearLog();
            SetLog("START");

            while (!stop_worker)
            {
                try
                {
                    if (data_mode == 0)
                    {
                        sendbuffer[0] = (byte)'B';
                        port.Write(sendbuffer, 0, 1);
                        port.Read(readbuffer, 0, 64);

                        if (readbuffer[0] == 'B')
                        {
                            int len = (readbuffer[1] << 8) + (readbuffer[2] << 0);
                            for (int i = 0; i < len; i++)
                            {
                                if (com_Buf_pos < com_Buf.Length)
                                {
                                    com_Buf[com_Buf_pos] = readbuffer[i + 3];
                                    com_Buf_pos++;

                                    if (com_Buf_pos == 8)
                                    {
                                        string line = String.Format("{0:X4}: [{1:X2} {2:X2}] {3:X2} {4:X2} {5:X2} {6:X2} SC:{7:X2} RCRC:{8:X2} RPI:{9:X2} RXDT:{10:X2} LCSS:{11:X2} TC:{12:X2} AT:{13:X2} CC:{14:X2} ??:{15:X2} ST:{16:X2}", com_Buf[0] * 256 + com_Buf[1], com_Buf[2], com_Buf[3], com_Buf[4], com_Buf[5], com_Buf[6], com_Buf[7], (com_Buf[4] >> 0) & 0x03, (com_Buf[4] >> 2) & 0x01, (com_Buf[4] >> 3) & 0x01, (com_Buf[4] >> 4) & 0x0f, (com_Buf[5] >> 0) & 0x03, (com_Buf[5] >> 2) & 0x01, (com_Buf[5] >> 3) & 0x01, (com_Buf[5] >> 4) & 0x0f, (com_Buf[6] >> 2) & 0x01, (com_Buf[7] >> 0) & 0x03);
                                        SetLog(line);

                                        com_Buf_pos = 0;
                                    }
                                }
                            }
                        }
                    }
                    else if ((data_mode == 1) || (data_mode == 2))
                    {
                        int size = (data_start + data_length) - data_pos;
                        if (size > 0)
                        {
                            if (size > 32)
                            {
                                size = 32;
                            }

                            sendbuffer[0] = (byte)'R';
                            sendbuffer[1] = (byte)data_mode;
                            sendbuffer[2] = (byte)((data_pos >> 24) & 0xFF);
                            sendbuffer[3] = (byte)((data_pos >> 16) & 0xFF);
                            sendbuffer[4] = (byte)((data_pos >> 8) & 0xFF);
                            sendbuffer[5] = (byte)((data_pos >> 0) & 0xFF);
                            sendbuffer[6] = (byte)((size >> 8) & 0xFF);
                            sendbuffer[7] = (byte)((size >> 0) & 0xFF);
                            port.Write(sendbuffer, 0, 8);
                            while (port.BytesToRead==0)
                            {
                                Thread.Sleep(0);
                            }
                            port.Read(readbuffer, 0, 64);

                            if (readbuffer[0] == 'R')
                            {
                                int len = (readbuffer[1] << 8) + (readbuffer[2] << 0);
                                for (int i = 0; i < len; i++)
                                {
                                    fileStream.WriteByte(readbuffer[i + 3]);
                                }
                                fileStream.Flush();

                                int progress = (data_pos-data_start)*100/data_length;
                                if (old_progress!=progress)
                                {
                                    SetLog(String.Format("{0}%", progress));
                                    old_progress = progress;
                                }

                                data_pos = data_pos + len;
                            }
                            else
                            {
                                SetLog(String.Format("read stopped (error at {0:X8})", data_pos));
                                close_data_mode();
                            }
                        }
                        else
                        {
                            SetLog("read finished");
                            close_data_mode();
                        }
                    }
                    else if (data_mode == 3)
                    {
                        int size = (data_start + data_length) - data_pos;
                        if (size > 0)
                        {
                            if (size > 32)
                            {
                                size = 32;
                            }

                            if (data_sector == -1)
                            {
                                if (!prepare_sector(data_pos, ref sendbuffer, ref readbuffer))
                                {
                                    SetLog(String.Format("write stopped (prepare sector error at {0:X8})", data_pos));
                                    close_data_mode();
                                };
                            }

                            if (data_mode != 0)
                            {
                                int len = 0;
                                for (int i = 0; i < size; i++)
                                {
                                    sendbuffer[i + 8] = (byte)fileStream.ReadByte();
                                    len++;

                                    if (data_sector != ((data_pos + len) / 4096))
                                    {
                                        break;
                                    }
                                }

                                if (send_data(data_pos, len, ref sendbuffer, ref readbuffer))
                                {
                                    int progress = (data_pos - data_start) * 100 / data_length;
                                    if (old_progress != progress)
                                    {
                                        SetLog(String.Format("{0}%", progress));
                                        old_progress = progress;
                                    }

                                    data_pos = data_pos + len;

                                    if (data_sector != (data_pos / 4096))
                                    {
                                        if (!write_sector(ref sendbuffer, ref readbuffer))
                                        {
                                            SetLog(String.Format("write stopped (write sector error at {0:X8})", data_pos));
                                            close_data_mode();
                                        };
                                    }
                                }
                                else
                                {
                                    SetLog(String.Format("write stopped (send data error at {0:X8})", data_pos));
                                    close_data_mode();
                                }
                            }
                        }
                        else
                        {
                            if (data_sector != -1)
                            {
                                if (!write_sector(ref sendbuffer, ref readbuffer))
                                {
                                    SetLog(String.Format("write stopped (write sector error at {0:X8})", data_pos));
                                };
                            }
                            SetLog("write finished");
                            close_data_mode();
                        }
                    }
                    else if (data_mode == 4)
                    {
                        int size = (data_start + data_length) - data_pos;
                        if (size > 0)
                        {
                            if (size > 32)
                            {
                                size = 32;
                            }

                            if (data_sector == -1)
                            {
                                data_sector = data_pos / 128;
                            }

                            int len = 0;
                            for (int i = 0; i < size; i++)
                            {
                                sendbuffer[i + 8] = (byte)fileStream.ReadByte();
                                len++;

                                if (data_sector != ((data_pos + len) / 128))
                                {
                                    data_sector = -1;
                                    break;
                                }
                            }

                            sendbuffer[0] = (byte)'W';
                            sendbuffer[1] = 4;
                            sendbuffer[2] = (byte)((data_pos >> 24) & 0xFF);
                            sendbuffer[3] = (byte)((data_pos >> 16) & 0xFF);
                            sendbuffer[4] = (byte)((data_pos >> 8) & 0xFF);
                            sendbuffer[5] = (byte)((data_pos >> 0) & 0xFF);
                            sendbuffer[6] = (byte)((len >> 8) & 0xFF);
                            sendbuffer[7] = (byte)((len >> 0) & 0xFF);
                            port.Write(sendbuffer, 0, len + 8);
                            while (port.BytesToRead == 0)
                            {
                                Thread.Sleep(0);
                            }
                            port.Read(readbuffer, 0, 64);

                            if ((readbuffer[0] == sendbuffer[0]) && (readbuffer[1] == sendbuffer[1]))
                            {
                                int progress = (data_pos - data_start) * 100 / data_length;
                                if (old_progress != progress)
                                {
                                    SetLog(String.Format("{0}%", progress));
                                    old_progress = progress;
                                }

                                data_pos = data_pos + len;
                            }
                            else
                            {
                                SetLog(String.Format("write stopped (send data error at {0:X8})", data_pos));
                                close_data_mode();
                            }
                        }
                        else
                        {
                            SetLog("write finished");
                            close_data_mode();
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                    break;
                }

                if ((data_mode != 1) && (data_mode != 2) && (data_mode != 3) && (data_mode != 4))
                {
                    Thread.Sleep(10);
                }
            }

            SetLog("STOP");
        }

        void worker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            port.Close();

            buttonStartStop.Text = "Start";
            checkBoxLogToFile.Enabled = true;
            comboBoxCOMPorts.Enabled = true;
            buttonRefreshCOMPortlist.Enabled = true;
            buttonReadFlash.Enabled = false;
            buttonWriteFlash.Enabled = false;
            buttonReadEEPROM.Enabled = false;
            buttonWriteEEPROM.Enabled = false;
            if (checkBoxLogToFile.Checked)
            {
                writer.Close();
            }
            running = false;

            if (form_close)
            {
                Close();
            }
        }

        private void buttonStartStop_Click(object sender, EventArgs e)
        {
            if (!running)
            {
                try
                {
                    port = new SerialPort(comboBoxCOMPorts.Text, 115200, Parity.None, 8, StopBits.One);
                    port.ReadTimeout = 1000;
                    port.Open();

                    worker = new BackgroundWorker();
                    worker.DoWork += new DoWorkEventHandler(worker_DoWork);
                    worker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(worker_RunWorkerCompleted);
                    worker.RunWorkerAsync();

                    buttonStartStop.Text = "Stop";
                    checkBoxLogToFile.Enabled = false;
                    comboBoxCOMPorts.Enabled = false;
                    buttonRefreshCOMPortlist.Enabled = false;
                    buttonReadFlash.Enabled = true;
                    buttonWriteFlash.Enabled = true;
                    buttonReadEEPROM.Enabled = true;
                    buttonWriteEEPROM.Enabled = true;
                    if (checkBoxLogToFile.Checked)
                    {
                        writer = new StreamWriter("log.txt");
                    }
                    running = true;
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message);
                }
            }
            else
            {
                stop_worker = true;
            }
        }

        private void buttonRefreshCOMPortlist_Click(object sender, EventArgs e)
        {
            loadCOMPortlist();
        }

        private void close_data_mode()
        {
            fileStream.Close();
            data_mode = 0;
        }

        private bool check_data_fields(bool check_length)
        {
            if (!int.TryParse(textBoxDataStart.Text, System.Globalization.NumberStyles.HexNumber, null, out data_start))
            {
                MessageBox.Show("ERROR: Please check 'start'.");
                return false;
            }

            if (check_length)
            {
                if (!int.TryParse(textBoxDataLength.Text, System.Globalization.NumberStyles.HexNumber, null, out data_length))
                {
                    MessageBox.Show("ERROR: Please check 'length'.");
                    return false;
                }
            }

            return true;
        }

        private void buttonReadFlash_Click(object sender, EventArgs e)
        {
            if (check_data_fields(true))
            {
                if (saveFileDialog.ShowDialog() == DialogResult.OK)
                {
                    saveFileDialog.InitialDirectory = Path.GetDirectoryName(saveFileDialog.FileName);
                    fileStream = saveFileDialog.OpenFile();
                    data_pos = data_start;
                    data_mode = 1;
                    old_progress = 0;
                    SetLog("read started");
                }
            }
        }

        private void buttonReadEEPROM_Click(object sender, EventArgs e)
        {
            if (check_data_fields(true))
            {
                if (saveFileDialog.ShowDialog() == DialogResult.OK)
                {
                    saveFileDialog.InitialDirectory = Path.GetDirectoryName(saveFileDialog.FileName);
                    fileStream = saveFileDialog.OpenFile();
                    data_pos = data_start;
                    data_mode = 2;
                    old_progress = 0;
                    SetLog("read started");
                }
            }
        }

        private void buttonWriteFlash_Click(object sender, EventArgs e)
        {
            if (check_data_fields(false))
            {
                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {
                    DialogResult dialogResult = MessageBox.Show("Do you really want to write to the external flash?", "External Flash Write", MessageBoxButtons.YesNo);
                    if (dialogResult == DialogResult.Yes)
                    {
                        openFileDialog.InitialDirectory = Path.GetDirectoryName(openFileDialog.FileName);
                        fileStream = openFileDialog.OpenFile();
                        data_pos = data_start;
                        data_length = (int)fileStream.Length;
                        data_mode = 3;
                        data_sector = -1;
                        old_progress = 0;
                        SetLog("write started");
                    }
                }
            }
        }

        private void buttonWriteEEPROM_Click(object sender, EventArgs e)
        {
            if (check_data_fields(false))
            {
                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {
                    DialogResult dialogResult = MessageBox.Show("Do you really want to write to the EEPROM?", "EEPROM Write", MessageBoxButtons.YesNo);
                    if (dialogResult == DialogResult.Yes)
                    {
                        openFileDialog.InitialDirectory = Path.GetDirectoryName(openFileDialog.FileName);
                        fileStream = openFileDialog.OpenFile();
                        data_pos = data_start;
                        data_length = (int)fileStream.Length;
                        data_mode = 4;
                        data_sector = -1;
                        old_progress = 0;
                        SetLog("write started");
                    }
                }
            }
        }
    }
}
