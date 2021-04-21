using System;

namespace UsbLibrary
{
	public class DataSendEventArgs : EventArgs
	{
		public readonly byte[] data;

		public DataSendEventArgs(byte[] data)
		{
			
			//base._002Ector();
			this.data = data;
		}
	}
}
