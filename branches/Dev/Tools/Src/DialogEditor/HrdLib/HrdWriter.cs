using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;

namespace HrdLib
{
    public class HrdWriter
    {
        internal const string UtcTimeFormatString = @"yyyyMMdd:hhmmss.fffffff", TimeFormatString = UtcTimeFormatString + "zzz";

        private readonly Stream _stream;
        private readonly Stack<HrdElement> _elementStack = new Stack<HrdElement>();

        public Stream Stream { get { return _stream; } }

        public HrdWriter(Stream stream)
        {
            _stream = stream;
            _elementStack.Push(new HrdDocument());
        }

        public void WriteBeginElement()
        {
            WriteBeginElement(null);
        }

        public void WriteBeginElement(string name)
        {
            var element = new HrdNode(name);
            _elementStack.Push(element);
        }

        public void WriteEndElement()
        {
            var element = (HrdNode) _elementStack.Pop();
            
            if (element.Name == null && element.ChildrenCount == 0)
                return;

            var parent = _elementStack.Peek();
            parent.AddElement(element);
        }

        public void WriteBeginArray()
        {
            WriteBeginArray(null);
        }

        public void WriteBeginArray(string name)
        {
            var array = new HrdArray(name);
            _elementStack.Push(array);
        }

        public void WriteEndArray()
        {
            var array = (HrdArray)_elementStack.Pop();
            var parent = _elementStack.Peek();
            parent.AddElement(array);
        }

        private void WriteValue(string value, bool quoted)
        {
            HrdAttribute attribute;
            var parent = _elementStack.Peek();
            if (parent is HrdNode)
                attribute = new HrdAttribute(parent.Name, value);
            else
                attribute = new HrdAttribute(null, value);
            attribute.SerializeAsQuotedString = quoted;

            parent.AddElement(attribute);
        }

        public void WriteValue(string value)
        {
            WriteValue(value, true);
        }

        public void WriteValue(Int64 value)
        {
            WriteValue(value.ToString(CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(UInt64 value)
        {
            WriteValue(value.ToString(CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(Int32 value)
        {
            WriteValue(value.ToString(CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(UInt32 value)
        {
            WriteValue(value.ToString(CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(Int16 value)
        {
            WriteValue(value.ToString(CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(UInt16 value)
        {
            WriteValue(value.ToString(CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(Byte value)
        {
            WriteValue(value.ToString(CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(SByte value)
        {
            WriteValue(value.ToString(CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(Char value)
        {
            WriteValue(new string(value, 1), true);
        }

        public void WriteValue(DateTime dateTime, bool ignoreTimeZone)
        {
            var formatString = ignoreTimeZone ? UtcTimeFormatString : TimeFormatString;
            WriteValue(dateTime.ToString(formatString, CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(Single value)
        {
            WriteValue(value.ToString("G", CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(Double value)
        {
            WriteValue(value.ToString("G", CultureInfo.InvariantCulture), false);
        }

        public void WriteValue(Boolean value)
        {
            WriteValue(value ? 1 : 0);
        }

        public void WriteValue(IFormatProvider provider, string format, params object[] args)
        {
            WriteValue(string.Format(provider, format, args));
        }

        public void WriteValue(string format, params object[] args)
        {
            WriteValue(CultureInfo.InvariantCulture, format, args);
        }

        public void Close()
        {
            var document = (HrdDocument) _elementStack.Pop();
            document.WriteDocument(_stream);
        }
    }
}
