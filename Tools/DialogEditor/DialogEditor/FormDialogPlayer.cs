using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Windows.Forms;
using DialogLogic;

namespace DialogDesigner
{
    public partial class FormDialogPlayer : Form
    {
        private class DialogNodeColor
        {
            public DialogGraphPhraseNodeBase Node { get; set; }
            public NodeColor Color { get; set; }

            public Color SystemColor
            {
                get
                {
                    switch(Color)
                    {
                        case NodeColor.Answer:
                            return System.Drawing.Color.LightSkyBlue;

                        case NodeColor.Question:
                            return System.Drawing.Color.LightCoral;

                        case NodeColor.ConditionalEnd:
                            return System.Drawing.Color.LightGray;

                        default:
                            return System.Drawing.Color.White;
                    }
                }
            }

            public enum NodeColor
            {
                Default=0,
                Answer,
                Phrase,
                Question,
                ConditionalEnd
            }
        }

        private const int CharPerSec=14;

        private readonly DialogGraph _graph;
        private readonly Stack<DialogGraphNodeBase> _path = new Stack<DialogGraphNodeBase>();

        private Timer _timer;

        public FormDialogPlayer(DialogInfo dialog)
        {
            Debug.Assert(dialog != null);

            InitializeComponent();
            _playerProgress.Visible = false;

            _graph = dialog.Graph;
            _path.Push(_graph.RootNode);
            
            LoadControls();
        }

        private void LoadControls()
        {
            if(_timer!=null)
            {
                _timer.Stop();
                _timer.Tag = null;
                _playerProgress.Visible = false;
            }

            DialogGraphNodeBase node = _path.Peek();

            int controlCount = 0;
            if(node==null)
                SetTableSize(0);
            else
            {
                var nextNodes = GetNextNodes(node);
                SetTableSize(nextNodes.Count);
                if(nextNodes.Count>0)
                {
                    controlCount = nextNodes.Count;
                    int i = 0;
                    for (int row = 0; row < _tableDialogs.RowCount; row++)
                    {
                        for (int col = 0; col < _tableDialogs.ColumnCount; col++, i++)
                        {
                            var control = (ControlPlayerNode) _tableDialogs.GetControlFromPosition(col, row);

                            if (i >= nextNodes.Count)
                            {
                                if (control != null)
                                {
                                    control.DialogNode = null;
                                    control.Tag = null;
                                    control.Visible = false;
                                }
                                continue;
                            }

                            if (control == null)
                            {
                                control = new ControlPlayerNode {Dock = DockStyle.Fill};
                                control.PhraseClick += OnPhraseClick;
                                _tableDialogs.Controls.Add(control, col, row);
                            }
                            control.DialogNode = nextNodes[i].Node;
                            control.Visible = true;
                            control.Tag = nextNodes[i].Node;
                            control.FieldColor = nextNodes[i].SystemColor;
                        }
                    }
                }
            }

            bool isAnswerNode = node is AnswerCollectionDialogGraphNode;
            _buttonNext.Enabled = node != null && controlCount > 0;
            _buttonPlay.Enabled = !isAnswerNode && controlCount > 0;
            if (controlCount>0 && node != null && _timer != null && !isAnswerNode)
            {
                _timer.Start();
                _playerProgress.Visible = true;
                _playerProgress.Value = 0;
            }

            _buttonBack.Enabled = _path.Count >= 2;
            _buttonRestart.Enabled = _path.Count >= 2;
        }

        private List<DialogNodeColor> GetNextNodes(DialogGraphNodeBase node)
        {
            var links = _graph.GetNodeLinks(node.Id);
            var result = new List<DialogNodeColor>();
            foreach(var link in links)
            {
                var dlgColor = new DialogNodeColor();
                if(link.Direction.ToNode==null)
                    continue;

                var nextNode = _graph.GetNode(link.Direction.ToNode.Value);
                if(nextNode is EmptyDialogGraphNode)
                {
                    result.AddRange(GetNextNodes(nextNode));
                    continue;
                }
                dlgColor.Node = (DialogGraphPhraseNodeBase) nextNode;
                if(nextNode is PhraseDialogGraphNode)
                {
                    if(node is PhraseDialogGraphNode || node is EmptyDialogGraphNode)
                        dlgColor.Color = DialogNodeColor.NodeColor.Phrase;
                    else if(node is AnswerCollectionDialogGraphNode)
                        dlgColor.Color = DialogNodeColor.NodeColor.Answer;
                }
                else if(nextNode is AnswerCollectionDialogGraphNode)
                    dlgColor.Color = DialogNodeColor.NodeColor.Question;
                result.Add(dlgColor);
            }
            return result;
        }

        private void OnPhraseClick(object sender, EventArgs e)
        {
            var node = (DialogGraphPhraseNodeBase)((Control)sender).Tag;
            _path.Push(node);
            LoadControls();
        }

        private void MoveNext(DialogGraphNodeBase node)
        {
            var next = GetNextNodes(node).Select(n => n.Node).FirstOrDefault();
            _path.Push(next);
            LoadControls();
        }

        private void SetTableSize(int controlCount)
        {
            int sqrt = (int)Math.Sqrt(controlCount);
            if (sqrt * sqrt < controlCount)
                sqrt++;

            int columns = sqrt, rows = sqrt == 0 ? 0 : controlCount / columns;
            if (rows * columns < controlCount)
                rows++;

            if(_tableDialogs.RowCount>rows || _tableDialogs.ColumnCount>columns)
            {
                for(int i=0; i<_tableDialogs.RowCount;i++)
                {
                    int j = (i >= rows) ? 0 : columns;
                    for(;j<_tableDialogs.ColumnCount;j++)
                    {
                        var control = _tableDialogs.GetControlFromPosition(j, i);
                        if(control!=null)
                            _tableDialogs.Controls.Remove(control);
                    }
                }
            }

            if (columns == 0)
                columns = 1;
            if (rows == 0)
                rows = 1;

            _tableDialogs.RowCount = rows;
            _tableDialogs.ColumnCount = columns;

            float percentSize = 100/(float) columns;

            for (int i = _tableDialogs.ColumnStyles.Count; i < _tableDialogs.ColumnCount; i++)
                _tableDialogs.ColumnStyles.Add(new ColumnStyle());

            foreach (ColumnStyle style in _tableDialogs.ColumnStyles)
            {
                style.SizeType = SizeType.Percent;
                style.Width = percentSize;
            }

            for (int i = _tableDialogs.RowStyles.Count; i < _tableDialogs.RowCount; i++)
                _tableDialogs.RowStyles.Add(new RowStyle());

            percentSize = 100/(float) rows;

            foreach(RowStyle style in _tableDialogs.RowStyles)
            {
                style.SizeType = SizeType.Percent;
                style.Height = percentSize;
            }
        }

        private void ButtonNextClick(object sender, EventArgs e)
        {
            var control = (ControlPlayerNode) _tableDialogs.GetControlFromPosition(0, 0);
            if(control!=null)
            {
                var link = (DialogGraphPhraseNodeBase)control.Tag;
                _path.Push(link);
                LoadControls();
            }
        }

        private void ButtonBackClick(object sender, EventArgs e)
        {
            if(_path.Count>1)
            {
                _path.Pop();
                LoadControls();
            }
        }

        private void ButtonRestartClick(object sender, EventArgs e)
        {
            while(_path.Count>1)
                _path.Pop();
            
            LoadControls();
        }

        private void ButtonPlayClick(object sender, EventArgs e)
        {
            if(_timer==null)
            {
                _timer=new Timer {Interval = 200};
                _timer.Tick += OnTimerTick;
                _buttonPlay.Image = Properties.Resources.media_pause_32;

                if (_tableDialogs.RowCount == 1 && _tableDialogs.ColumnCount==1)
                {
                    _playerProgress.Value = 0;
                    _playerProgress.Visible = true;
                    _timer.Start();
                }
            }
            else
            {
                _timer.Stop();
                _timer.Tick -= OnTimerTick;
                _timer.Dispose();
                _timer = null;
                _buttonPlay.Image = Properties.Resources.media_play_32;
                _playerProgress.Visible = false;
            }
        }

        private void OnTimerTick(object sender, EventArgs e)
        {
            var control = _tableDialogs.GetControlFromPosition(0, 0);
            var link = (DialogGraphPhraseNodeBase)control.Tag;
            int count = link.Phrase == null ? 0 : link.Phrase.Length;
            if(count<CharPerSec)
                count = CharPerSec;

            var timer = ((Timer) sender);
            float current = (timer.Tag as float?) ?? 0F;

            if (current > count)
            {
                _playerProgress.Visible = false;
                MoveNext(_path.Peek());
                return;
            }

            current += timer.Interval/1000F*CharPerSec;

            _playerProgress.Maximum = count;
            int val = (int) current;
            if(val>count)
                val = count;
            _playerProgress.Value = val;
            timer.Tag = current;
        }
    }
}
