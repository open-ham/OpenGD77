using System;

namespace UsbLibrary
{
	public class SpecifiedOutputReport : OutputReport
	{
		public SpecifiedOutputReport(HIDDevice oDev) : base(oDev)
		{
			
		}

		public bool SendData(byte[] data)
		{
			byte[] buffer = base.Buffer;
			buffer[0] = 1;
			buffer[1] = 0;
			buffer[2] = Convert.ToByte(data.Length);
			buffer[3] = Convert.ToByte(data.Length >> 8);
			Array.Copy(data, 0, buffer, 4, Math.Min(data.Length, base.Buffer.Length - 4));
			if (buffer.Length < data.Length)
			{
				return false;
			}
			return true;
		}

		public bool SendData(byte[] data, int index, int length)
		{
			byte[] buffer = base.Buffer;
			buffer[0] = 1;
			buffer[1] = 0;
			buffer[2] = Convert.ToByte(data.Length);
			buffer[3] = Convert.ToByte(data.Length >> 8);
			Array.Copy(data, index, buffer, 4, Math.Min(length, base.Buffer.Length - 4));
			if (buffer.Length < data.Length)
			{
				return false;
			}
			return true;
		}
	}
}
