namespace UsbLibrary
{
	public abstract class Report
	{
		private byte[] m_arrBuffer;

		private int m_nLength;

		public byte[] Buffer
		{
			get
			{
				return this.m_arrBuffer;
			}
			set
			{
				this.m_arrBuffer = value;
			}
		}

		public int BufferLength
		{
			get
			{
				return this.m_nLength;
			}
		}


		public Report(HIDDevice oDev)
		{
			
		}

		protected void SetBuffer(byte[] arrBytes)
		{
			this.m_arrBuffer = arrBytes;
			this.m_nLength = this.m_arrBuffer.Length;
		}
	}
}
