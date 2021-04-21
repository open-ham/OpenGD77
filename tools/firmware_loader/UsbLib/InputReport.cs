namespace UsbLibrary
{
	public abstract class InputReport : Report
	{


		public InputReport(HIDDevice oDev) : base(oDev)
		{
			
		}

		public void SetData(byte[] arrData)
		{
			base.SetBuffer(arrData);
			this.ProcessData();
		}

		public abstract void ProcessData();
	}
}
