namespace UsbLibrary
{
	public class SpecifiedInputReport : InputReport
	{
		private byte[] arrData;

		public byte[] Data
		{
			get
			{
				return this.arrData;
			}
		}

		public SpecifiedInputReport(HIDDevice oDev) : base (oDev)
		{
			
		}

		public override void ProcessData()
		{
			this.arrData = base.Buffer;
		}
	}
}
