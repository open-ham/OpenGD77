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
 
namespace ComTool
{
    partial class FormMain
    {
        /// <summary>
        /// Erforderliche Designervariable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Verwendete Ressourcen bereinigen.
        /// </summary>
        /// <param name="disposing">True, wenn verwaltete Ressourcen gelöscht werden sollen; andernfalls False.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Vom Windows Form-Designer generierter Code

        /// <summary>
        /// Erforderliche Methode für die Designerunterstützung.
        /// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
        /// </summary>
        private void InitializeComponent()
        {
            this.buttonStartStop = new System.Windows.Forms.Button();
            this.labelStatus = new System.Windows.Forms.Label();
            this.labelStatusText = new System.Windows.Forms.Label();
            this.comboBoxCOMPorts = new System.Windows.Forms.ComboBox();
            this.buttonRefreshCOMPortlist = new System.Windows.Forms.Button();
            this.richTextBoxLog = new System.Windows.Forms.RichTextBox();
            this.checkBoxLogToFile = new System.Windows.Forms.CheckBox();
            this.textBoxDataStart = new System.Windows.Forms.TextBox();
            this.labelDataStart = new System.Windows.Forms.Label();
            this.buttonReadFlash = new System.Windows.Forms.Button();
            this.labelDataLength = new System.Windows.Forms.Label();
            this.textBoxDataLength = new System.Windows.Forms.TextBox();
            this.buttonReadEEPROM = new System.Windows.Forms.Button();
            this.buttonWriteEEPROM = new System.Windows.Forms.Button();
            this.buttonWriteFlash = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // buttonStartStop
            // 
            this.buttonStartStop.Location = new System.Drawing.Point(15, 12);
            this.buttonStartStop.Name = "buttonStartStop";
            this.buttonStartStop.Size = new System.Drawing.Size(75, 23);
            this.buttonStartStop.TabIndex = 0;
            this.buttonStartStop.Text = "Start";
            this.buttonStartStop.UseVisualStyleBackColor = true;
            this.buttonStartStop.Click += new System.EventHandler(this.buttonStartStop_Click);
            // 
            // labelStatus
            // 
            this.labelStatus.AutoSize = true;
            this.labelStatus.Location = new System.Drawing.Point(12, 428);
            this.labelStatus.Name = "labelStatus";
            this.labelStatus.Size = new System.Drawing.Size(40, 13);
            this.labelStatus.TabIndex = 2;
            this.labelStatus.Text = "Status:";
            // 
            // labelStatusText
            // 
            this.labelStatusText.AutoSize = true;
            this.labelStatusText.Location = new System.Drawing.Point(58, 428);
            this.labelStatusText.Name = "labelStatusText";
            this.labelStatusText.Size = new System.Drawing.Size(22, 13);
            this.labelStatusText.TabIndex = 3;
            this.labelStatusText.Text = "OK";
            // 
            // comboBoxCOMPorts
            // 
            this.comboBoxCOMPorts.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.comboBoxCOMPorts.FormattingEnabled = true;
            this.comboBoxCOMPorts.Location = new System.Drawing.Point(586, 12);
            this.comboBoxCOMPorts.Name = "comboBoxCOMPorts";
            this.comboBoxCOMPorts.Size = new System.Drawing.Size(121, 21);
            this.comboBoxCOMPorts.TabIndex = 2;
            // 
            // buttonRefreshCOMPortlist
            // 
            this.buttonRefreshCOMPortlist.Location = new System.Drawing.Point(713, 11);
            this.buttonRefreshCOMPortlist.Name = "buttonRefreshCOMPortlist";
            this.buttonRefreshCOMPortlist.Size = new System.Drawing.Size(75, 23);
            this.buttonRefreshCOMPortlist.TabIndex = 3;
            this.buttonRefreshCOMPortlist.Text = "Refresh";
            this.buttonRefreshCOMPortlist.UseVisualStyleBackColor = true;
            this.buttonRefreshCOMPortlist.Click += new System.EventHandler(this.buttonRefreshCOMPortlist_Click);
            // 
            // richTextBoxLog
            // 
            this.richTextBoxLog.Location = new System.Drawing.Point(15, 41);
            this.richTextBoxLog.Name = "richTextBoxLog";
            this.richTextBoxLog.ReadOnly = true;
            this.richTextBoxLog.ScrollBars = System.Windows.Forms.RichTextBoxScrollBars.ForcedVertical;
            this.richTextBoxLog.Size = new System.Drawing.Size(773, 384);
            this.richTextBoxLog.TabIndex = 10;
            this.richTextBoxLog.Text = "";
            // 
            // checkBoxLogToFile
            // 
            this.checkBoxLogToFile.AutoSize = true;
            this.checkBoxLogToFile.Location = new System.Drawing.Point(96, 14);
            this.checkBoxLogToFile.Name = "checkBoxLogToFile";
            this.checkBoxLogToFile.Size = new System.Drawing.Size(72, 17);
            this.checkBoxLogToFile.TabIndex = 1;
            this.checkBoxLogToFile.Text = "Log to file";
            this.checkBoxLogToFile.UseVisualStyleBackColor = true;
            // 
            // textBoxDataStart
            // 
            this.textBoxDataStart.Location = new System.Drawing.Point(845, 11);
            this.textBoxDataStart.Name = "textBoxDataStart";
            this.textBoxDataStart.Size = new System.Drawing.Size(100, 20);
            this.textBoxDataStart.TabIndex = 4;
            // 
            // labelDataStart
            // 
            this.labelDataStart.AutoSize = true;
            this.labelDataStart.Location = new System.Drawing.Point(799, 16);
            this.labelDataStart.Name = "labelDataStart";
            this.labelDataStart.Size = new System.Drawing.Size(32, 13);
            this.labelDataStart.TabIndex = 10;
            this.labelDataStart.Text = "Start:";
            // 
            // buttonReadFlash
            // 
            this.buttonReadFlash.Enabled = false;
            this.buttonReadFlash.Location = new System.Drawing.Point(799, 70);
            this.buttonReadFlash.Name = "buttonReadFlash";
            this.buttonReadFlash.Size = new System.Drawing.Size(100, 23);
            this.buttonReadFlash.TabIndex = 6;
            this.buttonReadFlash.Text = "Read Flash";
            this.buttonReadFlash.UseVisualStyleBackColor = true;
            this.buttonReadFlash.Click += new System.EventHandler(this.buttonReadFlash_Click);
            // 
            // labelDataLength
            // 
            this.labelDataLength.AutoSize = true;
            this.labelDataLength.Location = new System.Drawing.Point(799, 44);
            this.labelDataLength.Name = "labelDataLength";
            this.labelDataLength.Size = new System.Drawing.Size(43, 13);
            this.labelDataLength.TabIndex = 13;
            this.labelDataLength.Text = "Length:";
            // 
            // textBoxDataLength
            // 
            this.textBoxDataLength.Location = new System.Drawing.Point(845, 41);
            this.textBoxDataLength.Name = "textBoxDataLength";
            this.textBoxDataLength.Size = new System.Drawing.Size(100, 20);
            this.textBoxDataLength.TabIndex = 5;
            // 
            // buttonReadEEPROM
            // 
            this.buttonReadEEPROM.Enabled = false;
            this.buttonReadEEPROM.Location = new System.Drawing.Point(799, 99);
            this.buttonReadEEPROM.Name = "buttonReadEEPROM";
            this.buttonReadEEPROM.Size = new System.Drawing.Size(100, 23);
            this.buttonReadEEPROM.TabIndex = 8;
            this.buttonReadEEPROM.Text = "Read EEPROM";
            this.buttonReadEEPROM.UseVisualStyleBackColor = true;
            this.buttonReadEEPROM.Click += new System.EventHandler(this.buttonReadEEPROM_Click);
            // 
            // buttonWriteEEPROM
            // 
            this.buttonWriteEEPROM.Enabled = false;
            this.buttonWriteEEPROM.Location = new System.Drawing.Point(905, 99);
            this.buttonWriteEEPROM.Name = "buttonWriteEEPROM";
            this.buttonWriteEEPROM.Size = new System.Drawing.Size(100, 23);
            this.buttonWriteEEPROM.TabIndex = 9;
            this.buttonWriteEEPROM.Text = "Write EEPROM";
            this.buttonWriteEEPROM.UseVisualStyleBackColor = true;
            this.buttonWriteEEPROM.Click += new System.EventHandler(this.buttonWriteEEPROM_Click);
            // 
            // buttonWriteFlash
            // 
            this.buttonWriteFlash.Enabled = false;
            this.buttonWriteFlash.Location = new System.Drawing.Point(905, 70);
            this.buttonWriteFlash.Name = "buttonWriteFlash";
            this.buttonWriteFlash.Size = new System.Drawing.Size(100, 23);
            this.buttonWriteFlash.TabIndex = 7;
            this.buttonWriteFlash.Text = "Write Flash";
            this.buttonWriteFlash.UseVisualStyleBackColor = true;
            this.buttonWriteFlash.Click += new System.EventHandler(this.buttonWriteFlash_Click);
            // 
            // FormMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(1016, 450);
            this.Controls.Add(this.buttonWriteEEPROM);
            this.Controls.Add(this.buttonWriteFlash);
            this.Controls.Add(this.buttonReadEEPROM);
            this.Controls.Add(this.labelDataLength);
            this.Controls.Add(this.textBoxDataLength);
            this.Controls.Add(this.buttonReadFlash);
            this.Controls.Add(this.labelDataStart);
            this.Controls.Add(this.textBoxDataStart);
            this.Controls.Add(this.checkBoxLogToFile);
            this.Controls.Add(this.richTextBoxLog);
            this.Controls.Add(this.buttonRefreshCOMPortlist);
            this.Controls.Add(this.comboBoxCOMPorts);
            this.Controls.Add(this.labelStatusText);
            this.Controls.Add(this.labelStatus);
            this.Controls.Add(this.buttonStartStop);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "FormMain";
            this.Text = "ComTool";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.FormMain_FormClosing);
            this.Load += new System.EventHandler(this.FormMain_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button buttonStartStop;
        private System.Windows.Forms.Label labelStatus;
        private System.Windows.Forms.Label labelStatusText;
        private System.Windows.Forms.ComboBox comboBoxCOMPorts;
        private System.Windows.Forms.Button buttonRefreshCOMPortlist;
        private System.Windows.Forms.RichTextBox richTextBoxLog;
        private System.Windows.Forms.CheckBox checkBoxLogToFile;
        private System.Windows.Forms.TextBox textBoxDataStart;
        private System.Windows.Forms.Label labelDataStart;
        private System.Windows.Forms.Button buttonReadFlash;
        private System.Windows.Forms.Label labelDataLength;
        private System.Windows.Forms.TextBox textBoxDataLength;
        private System.Windows.Forms.Button buttonReadEEPROM;
        private System.Windows.Forms.Button buttonWriteEEPROM;
        private System.Windows.Forms.Button buttonWriteFlash;
    }
}

