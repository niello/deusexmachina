using System;
using System.Collections.Generic;
using System.IO;

namespace HrdLib
{
    public partial class HrdDocument:HrdElement
    {
        public HrdDocument(): base(null)
        {}

        public static HrdDocument Read(Stream stream)
        {
            var doc = new HrdDocument();
            using(var sReader=new StreamReader(stream))
            {
                try
                {
                    var statQueue = new Queue<Statement>();
                    var elementStack = new Stack<HrdElement>();
                    elementStack.Push(doc);

                    bool waitingForComma = false;
                    while (!sReader.EndOfStream)
                    {
                        var statement = sReader.ReadNextStatement();
                        if (statement == null)
                            continue;

                        statQueue.Enqueue(statement);

                        var first = statQueue.Peek();
                        if (waitingForComma)
                        {
                            if (first.Type == StatementType.Comma)
                            {
                                statQueue.Dequeue();
                                waitingForComma = false;
                                continue;
                            }
                            if (first.Type != StatementType.SquareBracketClosed &&
                                first.Type != StatementType.BraceClosed)
                                throw new Exception("Comma not found.");
                        }

                        switch (first.Type)
                        {
                            case StatementType.Value:
                            case StatementType.StringValue:
                                var attr = new HrdAttribute {Value = first.Value};
                                var elt = Peek(elementStack);
                                elt.AddElement(attr);
                                statQueue.Dequeue();
                                waitingForComma = elt is HrdArray;
                                break;

                            case StatementType.Name:
                                if (statQueue.Count >= 2)
                                {
                                    var name = statQueue.Dequeue();
                                    var nextStat = statQueue.Dequeue();
                                    Statement nextNextStat=null;
                                    if (statQueue.Count == 1)
                                        nextNextStat = statQueue.Dequeue();

                                    HrdElement newElement = null;
                                    switch (nextStat.Type)
                                    {
                                        case StatementType.EqualsSign:
                                            if (nextNextStat == null)
                                            {
                                                statQueue.Enqueue(name);
                                                statQueue.Enqueue(nextStat);
                                            }
                                            else if (nextNextStat.Type == StatementType.SquareBracketOpened)
                                            {
                                                goto case StatementType.SquareBracketOpened;
                                            }
                                            else if (nextNextStat.Type == StatementType.BraceOpened)
                                            {
                                                goto case StatementType.BraceOpened;
                                            }
                                            else if (nextNextStat.Type == StatementType.Value || nextNextStat.Type == StatementType.StringValue)
                                            {
                                                newElement = new HrdAttribute(name.Value, nextNextStat.Value);
                                            }
                                            else
                                                throw new Exception(string.Format("Unrecognized statement '{0}'.",
                                                                                  nextNextStat.Value));
                                            break;

                                        case StatementType.SquareBracketOpened:
                                            newElement = new HrdArray(name.Value);
                                            break;

                                        case StatementType.BraceOpened:
                                            newElement = new HrdNode(name.Value);
                                            break;

                                        default:
                                            throw new Exception("Unknown statement.");
                                    }

                                    if(newElement!=null)
                                    {
                                        var currEl = Peek(elementStack);
                                        currEl.AddElement(newElement);
                                        if (!(newElement is HrdAttribute))
                                            elementStack.Push(newElement);
                                    }
                                }
                                break;

                            case StatementType.SquareBracketOpened:
                                statQueue.Dequeue();
                                var arr = new HrdArray();
                                var parentElt = Peek(elementStack);
                                parentElt.AddElement(arr);
                                elementStack.Push(arr);
                                break;

                            case StatementType.BraceOpened:
                                statQueue.Dequeue();
                                var n = new HrdNode();
                                var parentNode = Peek(elementStack);
                                parentNode.AddElement(n);
                                elementStack.Push(n);
                                break;

                            case StatementType.Comma:
                                var pEl = Peek(elementStack);
                                if (pEl is HrdArray)
                                    pEl.AddElement(null);
                                else
                                    throw new Exception("Comma is not recognized.");
                                statQueue.Dequeue();
                                break;

                            case StatementType.SquareBracketClosed:
                                var guessItsArray = Pop(elementStack);
                                if (!(guessItsArray is HrdArray))
                                    throw new Exception("There is no end of array.");

                                waitingForComma = Peek(elementStack) is HrdArray;
                                statQueue.Dequeue();
                                break;

                            case StatementType.BraceClosed:
                                var guessItsNode = Pop(elementStack);
                                if (!(guessItsNode is HrdNode))
                                    throw new Exception("There is no end of node");

                                waitingForComma = Peek(elementStack) is HrdArray;
                                statQueue.Dequeue();
                                break;

                            default:
                                throw new Exception(string.Format("Unknown statement '{0}'.", statement.Value));
                        }
                    }

                    var root = Pop(elementStack);
                    if (elementStack.Count != 0 || !(root is HrdDocument))
                        throw new Exception("Document is corrupted.");
                    return (HrdDocument) root;
                }
                catch(Exception ex)
                {
                    throw new Exception(string.Format("Line {0}. {1}", sReader.LineCounter, ex.Message));
                }
            }
        }

        private static T Pop<T>(Stack<T> stack)
        {
            if (stack.Count == 0)
                throw new Exception("No elements found.");

            return stack.Pop();
        }

        private static T Peek<T>(Stack<T> stack)
        {
            if (stack.Count == 0)
                throw new Exception("No elements found.");

            return stack.Peek();
        }

        public void WriteDocument(Stream stream)
        {
            using (var writer = new StreamWriter(stream, false))
            {
                foreach (var element in GetElements())
                    WriteElement(element, writer);
            }
        }

        private void WriteElement(HrdElement element, StreamWriter writer)
        {
            var statement = new Statement();
            if(element.Name!=null)
            {
                statement.Type = StatementType.Name;
                statement.Value = element.Name;
                writer.WriteStatement(statement);
                writer.WriteStatement(StatementType.EqualsSign);
            }

            if (element is HrdAttribute)
            {
                var attr = (HrdAttribute) element;
                statement.Value = attr.Value;
                statement.Type = attr.SerializeAsQuotedString ? StatementType.StringValue : StatementType.Value;
                writer.WriteStatement(statement);
            }
            else if (element is HrdArray)
            {
                writer.WriteStatement(StatementType.SquareBracketOpened);
                bool first = true;
                foreach (var child in element.GetElements())
                {
                    if (!first)
                        writer.WriteStatement(StatementType.Comma);
                    else
                        first = false;
                    WriteElement(child, writer);
                }

                writer.WriteStatement(StatementType.SquareBracketClosed);
            }
            else if (element is HrdNode)
            {
                bool isVirtual = ((HrdNode) element).IsVirtual;
                if (!isVirtual)
                    writer.WriteStatement(StatementType.BraceOpened);
                foreach (var child in element.GetElements())
                    WriteElement(child, writer);

                if (!isVirtual)
                    writer.WriteStatement(StatementType.BraceClosed);
            }
            else
                throw new Exception(string.Format("Element is '{0}'. This type is not supported.", element.GetType()));
        }
    }
}
