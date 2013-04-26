using System;
using System.Linq;
using System.IO;
using System.Text;

namespace HrdLib
{
    partial class HrdDocument
    {
        private class StreamReader : IDisposable
        {
            private static readonly char[] CharStatement = "\"[]{}, \t=;/".ToCharArray(),
                                           StatementDelimiters = " \t;".ToCharArray();

            private readonly System.IO.StreamReader _innerReader;
            private string _currentLine;
            private int _linePosition;
            private int _lineCounter;
            private Statement _nextStatement;

            public bool EndOfStream
            {
                get
                {
                    return _innerReader.EndOfStream &&
                           (string.IsNullOrEmpty(_currentLine) || _linePosition >= _currentLine.Length);
                }
            }

            public int LineCounter { get { return _lineCounter; } }

            public StreamReader(Stream stream)
            {
                _innerReader = new System.IO.StreamReader(stream, Encoding.UTF8);
            }

            public Statement ReadNextStatement()
            {
                if (_nextStatement != null)
                {
                    var stat = _nextStatement;
                    _nextStatement = null;
                    return stat;
                }

                if ((_currentLine == null || _linePosition >= _currentLine.Length) && !ReadNextLine())
                    return null;

                var idx = _currentLine.IndexOfAny(CharStatement, _linePosition);
                if (idx < 0 || idx > _linePosition)
                {
                    string str;
                    if (idx < 0)
                    {
                        str = _currentLine.Substring(_linePosition);
                        _linePosition = _currentLine.Length;
                    }
                    else
                    {
                        int length = idx - _linePosition;
                        str = _currentLine.Substring(_linePosition, length);
                        _linePosition = idx;
                    }

                    var st = new Statement {Value = str};
                    if (str[0] >= '0' && str[0] <= '9')
                        st.Type = StatementType.Value;
                    else
                        st.Type = StatementType.Name;
                    return st;
                }

                var ch = _currentLine[idx];

                if (ch == '/' && idx != _currentLine.Length - 1)
                {
                    if (_currentLine[idx + 1] == '/')
                    {
                        _linePosition = _currentLine.Length;
                        return ReadNextStatement();
                    }
                    if (_currentLine[idx + 1] == '*')
                    {
                        const string endComment = "*/";
                        _linePosition += 2;
                        int endIdx;
                        do
                        {
                            if (_linePosition <= _currentLine.Length - 2)
                                endIdx = _currentLine.IndexOf(endComment, _linePosition);
                            else
                                endIdx = -1;
                            if (endIdx >= 0)
                                break;
                        } while (ReadNextLine());

                        if (endIdx < 0)
                            return null;

                        _linePosition = endIdx + endComment.Length;
                        return ReadNextStatement();
                    }
                }

                if (StatementDelimiters.Contains(ch))
                {
                    do
                    {
                        idx++;
                    } while (idx < _currentLine.Length && StatementDelimiters.Contains(_currentLine[idx]));
                    _linePosition = idx;
                    return ReadNextStatement();
                }

                switch (ch)
                {
                    case '[':
                        _linePosition = idx + 1;
                        return new Statement(StatementType.SquareBracketOpened, "[");

                    case ']':
                        _linePosition = idx + 1;
                        return new Statement(StatementType.SquareBracketClosed, "]");

                    case '}':
                        _linePosition = idx + 1;
                        return new Statement(StatementType.BraceClosed, "}");

                    case '{':
                        _linePosition = idx + 1;
                        return new Statement(StatementType.BraceOpened, "{");

                    case '\"':
                        _linePosition = idx + 1;
                        string str = ReadString();
                        // concatenation of strings
                        var statement = ReadNextStatement();
                        if (statement != null && statement.Type == StatementType.Value && statement.Value != null)
                            str += statement.Value;
                        else
                            _nextStatement = statement;
                        return new Statement(StatementType.StringValue, str);

                    case ',':
                        _linePosition = idx + 1;
                        return new Statement(StatementType.Comma, ",");

                    case '=':
                        _linePosition = idx + 1;
                        return new Statement(StatementType.EqualsSign, "=");

                    default:
                        return new Statement(StatementType.Unknown, ch.ToString());

                }
            }

            private bool ReadNextLine()
            {
                while (true)
                {
                    if (_innerReader.EndOfStream)
                        return false;

                    string line = _innerReader.ReadLine();
                    _lineCounter++;
                    if (string.IsNullOrEmpty(line))
                        continue;

                    _currentLine = line;
                    _linePosition = 0;
                    break;
                }

                return true;
            }

            private string ReadString()
            {
                char[] specialChar = new[] { '\"', '\\' };
                var builder = new StringBuilder();
                bool closed = false;

                while (!closed)
                {
                    if (_linePosition >= _currentLine.Length)
                    {
                        //builder.AppendLine();
                        if (!ReadNextLine())
                            throw new Exception("Suddenly! The end of file.");
                        continue;
                    }

                    int specialCharIdx = _currentLine.IndexOfAny(specialChar, _linePosition);
                    if (specialCharIdx < 0)
                    {
                        builder.Append(_currentLine.Substring(_linePosition));
                        _linePosition = _currentLine.Length;
                        continue;
                    }

                    int len = specialCharIdx - _linePosition;
                    if (len > 0)
                        builder.Append(_currentLine.Substring(_linePosition, specialCharIdx - _linePosition));

                    var ch = _currentLine[specialCharIdx];
                    if (ch == '"')
                    {
                        _linePosition = specialCharIdx + 1;
                        break;
                    }

                    //if(ch=='\')

                    if (specialCharIdx == _currentLine.Length - 1)
                    {
                        builder.AppendLine();
                        _linePosition = _currentLine.Length;
                        continue;
                    }

                    var nextCh = _currentLine[specialCharIdx + 1];
                    switch (nextCh)
                    {
                        case '"':
                        case '\\':
                            builder.Append(nextCh);
                            break;

                        case 'n':
                            builder.AppendLine();
                            break;

                        default:
                            builder.Append(nextCh);
                            break;
                    }

                    _linePosition = specialCharIdx + 2;
                }

                return builder.ToString();
            }

            public void Dispose()
            {
                if (_innerReader != null)
                    _innerReader.Dispose();
            }
        }

        private class StreamWriter : IDisposable
        {
            private readonly System.IO.StreamWriter _innerWriter;
            private char[] _tabChars;
            private int _level;

            public StreamWriter(Stream stream, bool disposable)
            {
                _innerWriter = disposable ? new System.IO.StreamWriter(stream, Encoding.UTF8) : new NotDisposingStreamWriter(stream, Encoding.UTF8);
                _tabChars = new char[4];
                for (int i = 0; i < _tabChars.Length; i++)
                    _tabChars[i] = '\t';
            }

            public void WriteStatement(StatementType type)
            {
                switch (type)
                {
                    case StatementType.Name:
                    case StatementType.Value:
                        throw new NotSupportedException(
                            "Name and value types is not supported by this method. Use WriteStatement(Statemnt) instead.");

                    case StatementType.BraceClosed:
                        _level--;
                        WriteNewLine();
                        _innerWriter.Write('}');
                        break;

                    case StatementType.BraceOpened:
                        WriteNewLine();
                        _level++;
                        _innerWriter.Write("{ ");
                        break;

                    case StatementType.Comma:
                        _innerWriter.Write(", ");
                        break;

                    case StatementType.EqualsSign:
                        _innerWriter.Write(" = ");
                        break;

                    case StatementType.SquareBracketClosed:
                        _level--;
                        WriteNewLine();
                        _innerWriter.Write(']');
                        break;
                    case StatementType.SquareBracketOpened:
                        WriteNewLine();
                        _level++;
                        _innerWriter.Write("[ ");
                        break;

                    default:
                        throw new Exception("Unknown statement.");
                }
            }

            public void WriteStatement(Statement statement)
            {
                switch (statement.Type)
                {
                    case StatementType.BraceClosed:
                    case StatementType.BraceOpened:
                    case StatementType.Comma:
                    case StatementType.EqualsSign:
                    case StatementType.SquareBracketClosed:
                    case StatementType.SquareBracketOpened:
                        WriteStatement(statement.Type);
                        break;

                    case StatementType.Name:
                        WriteNewLine();
                        _innerWriter.Write(statement.Value);
                        break;

                    case StatementType.Value:
                        _innerWriter.Write(statement.Value);
                        break;

                    case StatementType.StringValue:
                        WriteString(statement.Value);
                        break;

                    default:
                        throw new Exception("Unknown statement.");
                }
            }

            private void WriteString(string str)
            {
                if (str == null)
                    str = string.Empty;
                var builder = new StringBuilder(str.Length);
                builder.Append('"');

                foreach (var ch in str)
                {
                    switch (ch)
                    {
                        case '\\':
                            builder.Append("\\\\");
                            break;

                        case '"':
                            builder.Append("\\\"");
                            break;

                        case '\n':
                            builder.AppendLine("\\");
                            break;

                        case '\r':
                            break;

                        default:
                            builder.Append(ch);
                            break;
                    }
                }
                builder.Append("\"");

                _innerWriter.Write(builder.ToString());
            }

            private void WriteNewLine()
            {
                if (_level >= _tabChars.Length)
                {
                    int oldLen = _tabChars.Length;
                    Array.Resize(ref _tabChars, oldLen * 2);
                    for (int i = oldLen; i < _tabChars.Length; i++)
                        _tabChars[i] = '\t';
                }

                _innerWriter.WriteLine();
                if (_level > 0)
                    _innerWriter.Write(_tabChars, 0, _level);
            }

            public void Dispose()
            {
                if (_innerWriter != null)
                    _innerWriter.Dispose();
            }
        }

        internal class Statement
        {
            public StatementType Type { get; set; }
            public string Value { get; set; }
            public bool IsQuoted { get; set; }

            public Statement() { }

            public Statement(StatementType type, string value)
            {
                Type = type;
                Value = value;
            }
        }

        internal enum StatementType
        {
            Unknown,
            Value,
            Name,
            SquareBracketOpened,
            SquareBracketClosed,
            BraceOpened,
            BraceClosed,
            Comma,
            EqualsSign,
            StringValue
        }
    }
}
