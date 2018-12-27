using System;
using System.Diagnostics;
using System.Windows.Forms;
using System.Linq;
using DialogLogic;

namespace DialogDesigner
{
    public partial class ControlDialogTree : UserControl
    {
        private DialogInfo _dialogInfo;
        public DialogInfo Dialog
        {
            get { return _dialogInfo; }
            set
            {
                if (!Equals(_dialogInfo, value))
                {
                    _copyLink = null;
                    _dialogInfo = value;
                    OnDialogInfoChanged();
                }
                else
                    OnSelectedLinkChanged();
            }
        }

        public DialogGraphLink SelectedLink
        {
            get
            {
                if (_treeDialog.SelectedNode == null)
                    return null;
                var node = _treeDialog.SelectedNode.Tag as DialogGraphNodeBase;
                if(node!=null)
                    return node.DefaultLinkHere;

                return _treeDialog.SelectedNode.Tag as DialogGraphLink;
            }
        }

        public event EventHandler SelectedLinkChanged;

        private DialogGraphLink _copyLink;

        public ControlDialogTree()
        {
            InitializeComponent();

            Enabled = false;

            _treeDialog.ImageList=new ImageList();
            _treeDialog.ImageList.Images.Add("EmptyNode", Properties.Resources.element_gray);
            _treeDialog.ImageList.Images.Add("PhraseNode", Properties.Resources.element);
            _treeDialog.ImageList.Images.Add("AnswerNode", Properties.Resources.elements1);
            _treeDialog.ImageList.Images.Add("Link", Properties.Resources.element_next);
            _treeDialog.ImageList.Images.Add("EndLink", Properties.Resources.element_stop);
        }

        private void OnDialogInfoChanged()
        {
            ClearNodes();
            Enabled = _dialogInfo != null;
        
            if(Enabled)
            {
                if(_dialogInfo!=null)
                {
                    _treeDialog.Nodes.Add(CreateNode(_dialogInfo.Graph, _dialogInfo.Graph.RootNode));
                    _treeDialog.ExpandAll();
                }
            }
        }

        private static TreeNode CreateNode(DialogGraph graph, DialogGraphNodeBase dialogNode)
        {
            var res = new TreeNode {Text = dialogNode.DisplayName};

            if(dialogNode is EmptyDialogGraphNode)
                res.ImageKey = @"EmptyNode";
            else if(dialogNode is PhraseDialogGraphNode)
                res.ImageKey = @"PhraseNode";
            else if(dialogNode is AnswerCollectionDialogGraphNode)
                res.ImageKey = @"AnswerNode";
            res.SelectedImageKey = res.ImageKey;

            res.Tag = dialogNode;

            var links = graph.GetNodeLinks(dialogNode.Id);
            foreach(var link in links)
            {
                var childNode = graph.GetNodeTo(link.Direction);
                if (childNode!=null && childNode.DefaultLinkHere == link)
                    res.Nodes.Add(CreateNode(graph, childNode));
                else
                    res.Nodes.Add(CreateLinkNode(graph,link));
            }

            return res;
        }

        private static TreeNode CreateLinkNode(DialogGraph graph, DialogGraphLink link)
        {
            var linkNode = new TreeNode { Tag = link };
            
            if (link.Direction.ToNode == null)
            {
                linkNode.Text = @"[End]";
                linkNode.ImageKey = @"EndLink";
            }
            else
            {
                var childNode = graph.GetNode(link.Direction.ToNode.Value);
                linkNode.Text = childNode.DisplayName;
                linkNode.ImageKey = @"Link";
            }

            linkNode.SelectedImageKey = linkNode.ImageKey;
            return linkNode;
        }

        private void ClearNodes()
        {
            _treeDialog.Nodes.Clear();
        }

        private void CmTreeCopyAsLinkClick(object sender, EventArgs e)
        {
            var treeNode = _treeDialog.SelectedNode;
            var tag = treeNode.Tag;
            if(tag is DialogGraphLink)
                _copyLink = ((DialogGraphLink) tag);
            else if(tag is DialogGraphNodeBase)
                _copyLink = ((DialogGraphNodeBase) tag).DefaultLinkHere;
        }

        private void CmTreeCutClick(object sender, EventArgs e)
        {
            var node = _treeDialog.SelectedNode;
            var tag = node.Tag;

            if(tag is DialogGraphLink)
            {
                var link = (DialogGraphLink) tag;
                _dialogInfo.Graph.Unlink(link.Direction);
                _copyLink = link;
            }
            else if(tag is DialogGraphNodeBase)
            {
                var graphNode = (DialogGraphNodeBase)tag;
                _copyLink = graphNode.DefaultLinkHere;
                _dialogInfo.Graph.Unlink(graphNode.DefaultLinkHere.Direction);
                if (graphNode.DefaultLinkHere != null)
                    ChangeDefaultLink(graphNode);
            }

            node.Remove();
        }

        private void ChangeDefaultLink(DialogGraphNodeBase graphNode)
        {
            var path = _dialogInfo.Graph.GetDefaultPath(graphNode.Id);
            var node = path.Count == 0 ? /*no way*/null : _treeDialog.Nodes[0];
            while(path.Count>0)
            {
                var direction = path.Pop();

                if (path.Count > 0)
                {
                    var grNode = node.Tag as DialogGraphNodeBase;
                    if (grNode == null || grNode.Id != direction.FromNode)
                        return;

                    node =
                        node.Nodes.Cast<TreeNode>().FirstOrDefault(
                            n => n.Tag is DialogGraphNodeBase && ((DialogGraphNodeBase) n.Tag).Id == direction.ToNode);

                    if(node==null)
                        break;

                    continue;
                }

                node =
                    node.Nodes.Cast<TreeNode>().FirstOrDefault(
                        n => n.Tag is DialogGraphLink && ((DialogGraphLink) n.Tag).Direction == direction);
            }

            if(node==null)
                return;

            var parent = node.Parent;
            int idx = parent.Nodes.IndexOf(node);
            node.Remove();

            node = CreateNode(_dialogInfo.Graph, graphNode);
            parent.Nodes.Insert(idx, node);
            node.ExpandAll();
        }

        private void CmTreePasteClick(object sender, EventArgs e)
        {
            if(_copyLink==null)
                return;

            var grNode = _dialogInfo.Graph.GetNodeTo(_copyLink.Direction);
            var node = _treeDialog.SelectedNode;
            var grParentNode = node.Tag as DialogGraphNodeBase;
            if(grParentNode==null)
                return;

            var link=_dialogInfo.Graph.Link(grParentNode.Id, _copyLink.Direction.ToNode);
            link.Condition = _copyLink.Condition;
            link.Action = _copyLink.Action;

            if(grNode!=null && link==grNode.DefaultLinkHere)
            {
                node.Nodes.Add(CreateNode(_dialogInfo.Graph, grNode));
            }
            else
            {
                node.Nodes.Add(CreateLinkNode(_dialogInfo.Graph, link));
            }
            node.Expand();
        }

        private void CmTreeDeleteClick(object sender, EventArgs e)
        {
            if (MessageBox.Show(this,@"Do you want to delete this node?",@"Dialog tree",MessageBoxButtons.YesNo) == DialogResult.Yes)
            {
                var node = _treeDialog.SelectedNode;
                if(node.Tag is DialogGraphNodeBase)
                {
                    var grNode = (DialogGraphNodeBase) node.Tag;
                    _dialogInfo.Graph.Unlink(grNode.DefaultLinkHere.Direction);
                    if (grNode.DefaultLinkHere != null)
                        ChangeDefaultLink(grNode);
                }
                else if(node.Tag is DialogGraphLink)
                {
                    _dialogInfo.Graph.Unlink(((DialogGraphLink) node.Tag).Direction);
                }

                node.Remove();
            }
        }

        private void CmTreePlayClick(object sender, EventArgs e)
        {
            var dlg = new FormDialogPlayer(_dialogInfo);
            dlg.ShowDialog(this);
        }

        private void CmTreeOpening(object sender, System.ComponentModel.CancelEventArgs e)
        {
            bool nodeSelected = _treeDialog.SelectedNode != null,
                 refLinkSelected = nodeSelected && _treeDialog.SelectedNode.Tag is DialogGraphLink;

            var node = !nodeSelected || refLinkSelected ? null : (DialogGraphNodeBase) _treeDialog.SelectedNode.Tag;

            Debug.Assert(node != null || !nodeSelected || refLinkSelected);

            bool isRootNode = node != null && node.DefaultLinkHere == null;

            _cmTreeReplace.Enabled = refLinkSelected &&
                                     ((DialogGraphLink) _treeDialog.SelectedNode.Tag).Direction.ToNode.HasValue;
            _cmTreePaste.Enabled = nodeSelected && _copyLink != null && !refLinkSelected &&
                                   _dialogInfo.Graph.CanLink(new DialogGraphLinkDirection(node.Id,
                                                                                                _copyLink.Direction.
                                                                                                    ToNode));
            _cmTreeDelete.Enabled = nodeSelected && !isRootNode;
            _cmTreeCut.Enabled = nodeSelected && !isRootNode;
            _cmTreeCopyAsLink.Enabled = nodeSelected && !isRootNode;

            _cmTreeAddEmpty.Enabled = nodeSelected && !refLinkSelected && !(node is AnswerCollectionDialogGraphNode);
            _cmTreeAddAnswer.Enabled = nodeSelected && !refLinkSelected && !(node is AnswerCollectionDialogGraphNode);
            _cmTreeAdd.Enabled = nodeSelected && !refLinkSelected;

            _cmTreeAddLinkToEnd.Enabled = nodeSelected && !refLinkSelected && _dialogInfo.Graph.CanLink(node.Id, null);
        }

        private void TreeDialogNodeMouseClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            _treeDialog.SelectedNode = e.Node;
        }

        private void TreeDialogBeforeSelect(object sender, TreeViewCancelEventArgs e)
        {
            if (_treeDialog.SelectedNode != null)
                SetTextFromTag(_treeDialog.SelectedNode);
        }

        private void SetTextFromTag(TreeNode node)
        {
            string text;
            if (node.Tag is DialogGraphNodeBase)
            {
                text = ((DialogGraphNodeBase) node.Tag).DisplayName;
            }
            else if(node.Tag is DialogGraphLink)
            {
                var grNode = _dialogInfo.Graph.GetNodeTo(((DialogGraphLink) node.Tag).Direction);
                if (grNode != null)
                    text = grNode.DisplayName;
                else
                    text = "[End]";
            }
            else text = string.Empty;
            if (!string.Equals(text, node.Text))
                node.Text = text;
        }

        private void TreeDialogAfterSelect(object sender, TreeViewEventArgs e)
        {
            OnSelectedLinkChanged();
        }

        private void OnSelectedLinkChanged()
        {
            if (SelectedLinkChanged != null)
                SelectedLinkChanged(this, EventArgs.Empty);
        }

        private void CmTreeAddAnswerClick(object sender, EventArgs e)
        {
            AddNode((parentId, graph) => graph.AddAnswerCollectionNode(parentId));
        }

        private void CmTreeAddClick(object sender, EventArgs e)
        {
            AddNode((parentId, graph) => graph.AddPhraseNode(parentId));
        }

        private void CmTreeAddEmptyClick(object sender, EventArgs e)
        {
            AddNode((parentId, graph) => graph.AddEmptyNode(parentId));
        }

        private void AddNode(Func<int, DialogGraph,DialogGraphLink> linkFactory)
        {
            var treeNode = _treeDialog.SelectedNode;
            var parentNode = treeNode.Tag as DialogGraphNodeBase;
            if (parentNode == null)
                return;

            TreeNode newTreeNode=null;
            try
            {
                var newLink = linkFactory(parentNode.Id, _dialogInfo.Graph);
                var newNode = _dialogInfo.Graph.GetNode(newLink.Direction.ToNode.Value);
                newTreeNode = CreateNode(_dialogInfo.Graph, newNode);
                treeNode.Nodes.Add(newTreeNode);
                treeNode.Expand();
            }
            catch (Exception ex)
            {
                this.ShowError(ex.Message);
            }

            _treeDialog.SelectedNode = newTreeNode ?? treeNode;
        }

        private void CmTreeReplaceClick(object sender, EventArgs e)
        {
            var node = _treeDialog.SelectedNode;
            var link = node == null ? null : node.Tag as DialogGraphLink;
            if(link==null)
                return;

            var targetGraphNode = _dialogInfo.Graph.GetNode(link.Direction.ToNode.Value);
            var oldLink = targetGraphNode.DefaultLinkHere;

            try
            {
                _dialogInfo.Graph.SetDefaultLink(link.Direction);
            }
            catch(Exception ex)
            {
                this.ShowError(ex.Message);
                return;
            }

            var parent = node.Parent;
            int idx = parent.Nodes.IndexOf(node);
            node.Remove();

            var targetNode = GetTreeNode(link.Direction.ToNode.Value);
            if (targetNode != null)
            {
                var targetParent = targetNode.Parent;
                int targetIdx = targetParent.Nodes.IndexOf(targetNode);
                targetNode.Remove();

                parent.Nodes.Insert(idx, targetNode);

                var newNode = CreateLinkNode(_dialogInfo.Graph, oldLink);
                targetParent.Nodes.Insert(targetIdx, newNode);
            }
            else
            {
                parent.Nodes.Insert(idx, CreateNode(_dialogInfo.Graph, targetGraphNode));
            }
        }

        private void CmTreeAddLinkToEndClick(object sender, EventArgs e)
        {
            var selectedNode = _treeDialog.SelectedNode;
            try
            {
                var node = selectedNode.Tag as DialogGraphNodeBase;
                Debug.Assert(node != null);
                var link = _dialogInfo.Graph.Link(node.Id, null);
                var treeNode = CreateLinkNode(_dialogInfo.Graph, link);
                selectedNode.Nodes.Add(treeNode);
                selectedNode.Expand();
                _treeDialog.SelectedNode = treeNode;
            }
            catch(Exception ex)
            {
                this.ShowError(ex);
            }
        }

        private TreeNode GetTreeNode(int graphNodeId)
        {
            if (graphNodeId == _dialogInfo.Graph.RootNode.Id)
                return _treeDialog.Nodes[0];

            var path=_dialogInfo.Graph.GetDefaultPath(graphNodeId);
            if(path.Count==0)
                return null;

            var node = _treeDialog.Nodes[0];
            while (path.Count>0 && node!=null)
            {
                var direction = path.Pop();

                if (direction.FromNode != ((DialogGraphNodeBase)node.Tag).Id)
                    node = null;
                else
                {
                    node =
                        node.Nodes.Cast<TreeNode>().FirstOrDefault(
                            n =>
                            n.Tag is DialogGraphNodeBase &&
                            ((DialogGraphNodeBase) n.Tag).DefaultLinkHere.Direction == direction);
                }
            }

            return node;
        }
    }
}
