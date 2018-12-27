using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;

namespace HrdLib
{
    public class HrdReader
    {
        private struct ElementInfo
        {
            public readonly HrdElement Parent;
            public readonly HrdElement Element;
            public readonly int Index;

            public ElementInfo(int index, HrdElement element, HrdElement parent)
            {
                Index = index;
                Parent = parent;
                Element = element;
            }
        }

        private const string UtcTimeFormatString = HrdWriter.UtcTimeFormatString, TimeFormatString = HrdWriter.TimeFormatString;

        private readonly Stream _stream;
        private readonly Stack<ElementInfo> _elementStack = new Stack<ElementInfo>();

        public Stream Stream { get { return _stream; } }

        /// <summary>
        /// Indicates that the current element is <see cref="HrdAttribute"/> and it has a value.
        /// </summary>
        public bool HasValue
        {
            get
            {
                if (_elementStack.Count == 0)
                    return false;

                var element = _elementStack.Peek().Element as HrdAttribute;
                if (element == null)
                    return false;

                return element.Value != null;
            }
        }

        /// <summary>
        /// Number of children of the current element.
        /// </summary>
        public int ChildrenCount
        {
            get
            {
                if (_elementStack.Count == 0)
                    return 0;

                var element = _elementStack.Peek().Element;
                return element.ChildrenCount;
            }
        }

        /// <summary>
        /// The name of current element
        /// </summary>
        public string ElementName
        {
            get
            {
                if (_elementStack.Count == 0)
                    return null;
                var elementInfo = _elementStack.Peek();
                return elementInfo.Element.Name;
            }
        }

        public HrdReader(Stream stream)
        {
            if (stream == null)
                throw new ArgumentNullException("stream");

            _stream = stream;
            var document = HrdDocument.Read(_stream);
            _elementStack.Push(new ElementInfo(-1, document, null));
        }

        public bool ReadNextSibling()
        {
            var elementInfo = _elementStack.Pop();

            if (elementInfo.Parent == null)
                return false;

            int nextIndex = elementInfo.Index + 1;
            if (nextIndex >= elementInfo.Parent.ChildrenCount)
            {
                _elementStack.Push(elementInfo);
                return false;
            }

            var nextElement = elementInfo.Parent.GetElementAt(nextIndex);
            var nextElementInfo = new ElementInfo(nextIndex, nextElement, elementInfo.Parent);
            _elementStack.Push(nextElementInfo);

            return true;
        }

        public bool ReadBeginElement()
        {
            var elementInfo = _elementStack.Peek();
            if (elementInfo.Element is HrdArray || elementInfo.Element is HrdNode)
            {
                if (elementInfo.Element.ChildrenCount == 0)
                    return false;

                var childElement = elementInfo.Element.GetElementAt(0);
                var childInfo = new ElementInfo(0, childElement, elementInfo.Element);
                _elementStack.Push(childInfo);

                return true;
            }

            return false;
        }

        private string ReadString(bool quoted)
        {
            var elementInfo = _elementStack.Peek();
            var attribute = elementInfo.Element as HrdAttribute;
            if (attribute == null)
                throw new HrdStructureValidationException(SR.GetString(SR.NonAttributeValueCantBeRead));

            if (attribute.SerializeAsQuotedString == quoted)
                return attribute.Value;

            throw new HrdStructureValidationException(SR.GetString(SR.ValueTypeMismatch));
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
        }
    }
}
