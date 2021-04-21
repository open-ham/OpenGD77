using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace GD77_FirmwareLoader
{
	public partial class FrmProgress : Form
	{
		public FrmProgress()
		{
			InitializeComponent();
			this.CenterToScreen();
			this.BringToFront();
		}

		public void SetLabel(string txt)
		{
			this.BringToFront();
			this.lblMessage.Text = txt;
			this.lblMessage.Update();
			this.Refresh();
		}
		public void SetProgressPercentage(int perc)
		{
			this.BringToFront();
			this.progressBar1.Value = perc;
			this.progressBar1.Update();
			this.Refresh();
		}
	}
}
