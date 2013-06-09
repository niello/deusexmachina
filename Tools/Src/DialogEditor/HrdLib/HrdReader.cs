using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;

namespace HrdLib
{
    public class HrdReader
    {
        internal const string UtcTimeFormatString = @"yyyyMMdd:hhmmss.fffffff", TimeFormatString = UtcTimeFormatString + "zzz";

        private readonly Stream _stream;
        private readonly Stack<HrdElement> _elementStack = new Stack<HrdElement>();

        public Stream Stream { get { return _stream; } }

        public HrdReader(Stream stream)
        {
            if (stream == null)
                throw new ArgumentNullException("stream");

            _stream = stream;
        }

        private string ReadString(bool quoted)
        {
            throw new NotImplementedException();
        }

        public Int64 ReadInt64()
        {
            var str = ReadString(false);
            return Int64.Parse(str, NumberStyles.Integer | NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }

        public UInt64 ReadUInt64()
        {
            var str = ReadString(false);
            return UInt64.Parse(str, NumberStyles.Integer | NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }

        public Int32 ReadInt32()
        {
            var str = ReadString(false);
            return Int32.Parse(str, NumberStyles.Integer | NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }

        public UInt32 ReadUInt32()
        {
            var str = ReadString(false);
            return UInt32.Parse(str, NumberStyles.Integer | NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }

        public Int16 ReadInt16()
        {
            var str = ReadString(false);
            return Int16.Parse(str, NumberStyles.Integer | NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }

        public UInt16 ReadUInt16()
        {
            var str = ReadString(false);
            return UInt16.Parse(str, NumberStyles.Integer | NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }

        public Byte ReadByte()
        {
            var str = ReadString(false);
            return Byte.Parse(str, NumberStyles.Integer | NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }

        public SByte ReadSByte()
        {
            var str = ReadString(false);
            return SByte.Parse(str, NumberStyles.Integer | NumberStyles.HexNumber, CultureInfo.InvariantCulture);
        }

        public Char ReadChar()
        {
            var str = ReadString(true);
            return str[0];
        }

        public DateTime ReadDateTime(bool ignoreTimeZone)
        {
            var str = ReadString(false);
            var formatString = ignoreTimeZone ? HrdWriter.TimeFormatString : HrdWriter.UtcTimeFormatString;
            return DateTime.ParseExact(str, formatString, CultureInfo.InvariantCulture);
        }

        public Single ReadSingle()
        {
            var str = ReadString(false);
            return Single.Parse(str, NumberStyles.Float, CultureInfo.InvariantCulture);
        }

        public Double ReadDouble()
        {
            var str = ReadString(false);
            return Double.Parse(str, NumberStyles.Float, CultureInfo.InvariantCulture);
        }

        public Boolean ReadBoolean()
        {
            return ReadByte() != 0;
        }

        public string ReadString()
        {
            return ReadString(true);
        }

        public void Close()
        {
            throw new NotImplementedException();
        }
    }
}
