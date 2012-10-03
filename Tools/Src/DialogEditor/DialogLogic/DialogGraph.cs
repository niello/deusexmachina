using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using HrdLib;

namespace DialogLogic
{
    public class DialogGraph
    {
        private int _nodeIdentity;

        public event EventHandler GraphChanged;
        private bool _raiseChanged = true;

        private readonly Dictionary<int, DialogGraphNodeBase> _nodes = new Dictionary<int, DialogGraphNodeBase>();

        private readonly Dictionary<int, List<DialogGraphLink>> _links =
            new Dictionary<int, List<DialogGraphLink>>();

        private readonly EmptyDialogGraphNode _rootNode;
        public EmptyDialogGraphNode RootNode { get { return _rootNode; } }

        public DialogGraph()
        {
            _rootNode = new EmptyDialogGraphNode { Id = -1 };
            _nodes.Add(_rootNode.Id, _rootNode);
        }

        public DialogGraphLink AddPhraseNode(int parentNodeId)
        {
            if (!_nodes.ContainsKey(parentNodeId))
                throw new KeyNotFoundException(string.Format("There is no node with Id={0}.", parentNodeId));

            return AddNode(parentNodeId, new PhraseDialogGraphNode());
        }

        public DialogGraphLink AddEmptyNode(int parentNodeId)
        {
            DialogGraphNodeBase parentNode;

            if (!_nodes.TryGetValue(parentNodeId, out parentNode))
                throw new KeyNotFoundException(string.Format("There is no node with Id={0}.", parentNodeId));

            if (parentNode is AnswerCollectionDialogGraphNode)
                throw new Exception("Only phrase node can be added.");

            return AddNode(parentNodeId, new EmptyDialogGraphNode());
        }

        public DialogGraphLink AddAnswerCollectionNode(int parentNodeId)
        {
            DialogGraphNodeBase parentNode;

            if (!_nodes.TryGetValue(parentNodeId, out parentNode))
                throw new KeyNotFoundException(string.Format("There is no node with Id={0}.", parentNodeId));

            if (parentNode is AnswerCollectionDialogGraphNode)
                throw new Exception("Only phrase node can be added.");

            return AddNode(parentNodeId, new AnswerCollectionDialogGraphNode());
        }

        private DialogGraphLink AddNode(int parentNodeId, DialogGraphNodeBase graphNode)
        {
            graphNode.Id = _nodeIdentity++;
            List<DialogGraphLink> nodeLinks;
            if (!_links.TryGetValue(parentNodeId, out nodeLinks))
            {
                nodeLinks = new List<DialogGraphLink>();
                _links.Add(parentNodeId, nodeLinks);
            }

            var link = new DialogGraphLink(parentNodeId, graphNode.Id);
            nodeLinks.Add(link);
            graphNode.DefaultLinkHere = link;

            _nodes.Add(graphNode.Id, graphNode);

            graphNode.PropertyChanged += OnNodePropertyChanged;
            link.PropertyChanged += OnLinkPropertyChanged;
            OnGraphChanged();
            return link;
        }

        public int GetChildNodeCount(int nodeId)
        {
            if (!_nodes.ContainsKey(nodeId))
                throw new KeyNotFoundException(string.Format("There is no node with Id={0}.", nodeId));

            List<DialogGraphLink> links;
            return _links.TryGetValue(nodeId, out links) ? links.Count : 0;
        }

        public List<DialogGraphLink> GetNodeLinks(int nodeId)
        {
            if (!_nodes.ContainsKey(nodeId))
                throw new KeyNotFoundException(string.Format("There is no node with Id={0}.", nodeId));

            List<DialogGraphLink> links;
            if (!_links.TryGetValue(nodeId, out links))
                return new List<DialogGraphLink>();

            return new List<DialogGraphLink>(links);
        }

        public DialogGraphNodeBase GetNode(int nodeId)
        {
            return _nodes[nodeId];
        }

        public DialogGraphNodeBase GetNodeTo(DialogGraphLinkDirection direction)
        {
            if(direction.ToNode==null)
                return null;
            return GetNode(direction.ToNode.Value);
        }

        /// <summary>
        /// Remove a link but keep the node in the graph
        /// </summary>
        public bool Unlink(DialogGraphLinkDirection linkDirection)
        {
            List<DialogGraphLink> links;
            if (!_links.TryGetValue(linkDirection.FromNode, out links))
                return false;

            var link = links.FirstOrDefault(l => l.Direction == linkDirection);
            if (link == null)
                return false;

            links.Remove(link);
            link.PropertyChanged -= OnLinkPropertyChanged;

            DialogGraphNodeBase toNode;
            if (link.Direction.ToNode.HasValue)
            {
                if (_nodes.TryGetValue(link.Direction.ToNode.Value, out toNode) && toNode != null &&
                    link == toNode.DefaultLinkHere)
                {
                    var nodeLinks = GetLinksToNode(toNode.Id);
                    toNode.DefaultLinkHere = nodeLinks.FirstOrDefault(CanBeDefaultLink);
                }
            }

            OnGraphChanged();
            return true;
        }

        private bool CanBeDefaultLink(DialogGraphLink link)
        {
            Debug.Assert(link.Direction.ToNode != null);

            DialogGraphLink startLink = link;
            DialogGraphNodeBase targetNode;
            if (!_nodes.TryGetValue(link.Direction.ToNode.Value, out targetNode) || targetNode == null)
                return false;

            var directions = new HashSet<DialogGraphLinkDirection>();

            while (link != null)
            {
                if (link.Direction.ToNode == targetNode.Id)
                    link = startLink;

                if (!directions.Contains(link.Direction))
                    directions.Add(link.Direction);
                else
                    return false;

                DialogGraphNodeBase node;
                if (!_nodes.TryGetValue(link.Direction.FromNode, out node) || node == null)
                    return false;

                link = node.DefaultLinkHere;
            }

            return true;
        }

        private List<DialogGraphLink> GetLinksToNode(int nodeId)
        {
            return _links.SelectMany(pair => pair.Value).Where(l => l.Direction.ToNode == nodeId).ToList();
        }

        public Stack<DialogGraphLinkDirection> GetDefaultPath(int nodeId)
        {
            DialogGraphNodeBase node;

            if (!_nodes.TryGetValue(nodeId, out node))
                throw new KeyNotFoundException(string.Format("There is no node with Id={0}.", nodeId));

            var result = new Stack<DialogGraphLinkDirection>();
            while (node.DefaultLinkHere != null)
            {
                if (result.Contains(node.DefaultLinkHere.Direction))
                {
                    //cyclic link break
                    result.Clear();
                    break;
                }

                result.Push(node.DefaultLinkHere.Direction);
                if (!_nodes.TryGetValue(node.DefaultLinkHere.Direction.FromNode, out node))
                    break;
            }

            return result;
        }

        public DialogGraphLink Link(DialogGraphLinkDirection direction)
        {
            DialogGraphNodeBase node;

            if (!_nodes.TryGetValue(direction.FromNode, out node))
                throw new KeyNotFoundException(string.Format("There is no node with Id={0}.", direction.FromNode));

            DialogGraphNodeBase childNode=null;
            if (direction.ToNode.HasValue && !_nodes.TryGetValue(direction.ToNode.Value, out childNode))
                throw new KeyNotFoundException(string.Format("There is no node with Id={0}.", direction.ToNode));

            if (node is AnswerCollectionDialogGraphNode && (childNode == null || !(childNode is PhraseDialogGraphNode)))
                throw new Exception("Only phrase node can be linked.");

            List<DialogGraphLink> links;
            if (!_links.TryGetValue(direction.FromNode, out links))
            {
                links = new List<DialogGraphLink>();
                _links.Add(direction.FromNode, links);
            }
            else if (links.Any(l => l.Direction == direction))
                throw new Exception("Link is duplicated.");

            var link = new DialogGraphLink(direction.FromNode, direction.ToNode);
            if (childNode!=null && childNode.DefaultLinkHere == null)
                childNode.DefaultLinkHere = link;

            links.Add(link);
            link.PropertyChanged += OnLinkPropertyChanged;

            OnGraphChanged();
            return link;
        }

        public DialogGraphLink Link(int fromNodeId, int? toNodeId)
        {
            return Link(new DialogGraphLinkDirection(fromNodeId, toNodeId));
        }

        /// <summary>
        /// ! For deserialization usage only !
        /// </summary>
        internal void ForceInsertNode(DialogGraphNodeBase node)
        {
            if (node.Id >= _nodeIdentity)
                _nodeIdentity = node.Id + 1;

            _nodes.Add(node.Id, node);
        }

        public bool CanLink(DialogGraphLinkDirection direction)
        {
            DialogGraphNodeBase fromNode, toNode=null;

            if (!_nodes.TryGetValue(direction.FromNode, out fromNode) || (direction.ToNode.HasValue && !_nodes.TryGetValue(direction.ToNode.Value, out toNode)))
            {
                return false;
            }

            if (fromNode is AnswerCollectionDialogGraphNode && (toNode==null || !(toNode is PhraseDialogGraphNode)))
            {
                return false;
            }

            List<DialogGraphLink> links;
            if (!_links.TryGetValue(direction.FromNode, out links) || links == null || links.Count == 0)
                return true;

            return !links.Any(l => l.Direction == direction);
        }

        public bool CanLink(int fromNodeId, int? toNodeId)
        {
            return CanLink(new DialogGraphLinkDirection(fromNodeId, toNodeId));
        }

        public void SetDefaultLink(DialogGraphLinkDirection newLink)
        {
            Debug.Assert(newLink.ToNode!=null);

            DialogGraphNodeBase targetNode;
            if (!_nodes.TryGetValue(newLink.ToNode.Value, out targetNode) || targetNode == null)
                throw new KeyNotFoundException("Node not found.");

            List<DialogGraphLink> links;
            DialogGraphLink link = null;
            if (_links.TryGetValue(newLink.FromNode, out links) && _links != null)
            {
                link = links.FirstOrDefault(l => l.Direction == newLink);
            }

            if (link == null)
                throw new Exception("Link not found.");

            if (!CanBeDefaultLink(link))
                throw new Exception("Unable to set default link: no path to root found.");

            targetNode.DefaultLinkHere = link;
            OnGraphChanged();
        }

        public bool AnyNode<TNodeType>(Func<TNodeType, bool> condition)
            where TNodeType : DialogGraphNodeBase
        {
            return _nodes.Values.OfType<TNodeType>().Any(condition);
        }

        internal void LoadFromFile(string fPath, IDialogUserProperties mdReader)
        {
            _raiseChanged = false;
            try
            {
                using (var fStream = new FileStream(fPath, FileMode.Open, FileAccess.Read))
                {
                    var sDoc = HrdDocument.Read(fStream);
                    var sDialog = sDoc.GetElement<HrdNode>("Nodes");
                    var sLinks = sDoc.GetElement<HrdArray>("Links");
                    var sScript = sDoc.GetElement<HrdAttribute>("ScriptFile");
                    var sStartNode = sDoc.GetElement<HrdAttribute>("StartNode");

                    if (sScript != null && sScript.Value is string)
                    {
                        var str = (string) sScript.Value;
                        int idx = str.IndexOf(':');
                        if (idx > 0)
                        {
                            var subst = str.Substring(0, idx);
                            if (subst == "dlg" && idx != str.Length - 1)
                                mdReader.DialogScriptFile = str.Substring(idx + 1);
                        }
                    }

                    var nodeMapping = new Dictionary<string, int>();
                    if (sDialog != null)
                    {
                        foreach (var sNode in sDialog.GetElements<HrdNode>())
                        {
                            int type;

                            if (!sNode.TryGetAttributeValue("Type", out type) ||
                                !Enum.IsDefined(typeof (DialogGraphNodeType), type))
                                throw new Exception(string.Format("Node {0}: unknown type.", sNode.Name));

                            DialogGraphNodeBase graphNode;
                            switch ((DialogGraphNodeType) type)
                            {
                                case DialogGraphNodeType.Empty:
                                    graphNode = new EmptyDialogGraphNode();
                                    break;

                                case DialogGraphNodeType.Answers:
                                    graphNode = new AnswerCollectionDialogGraphNode();
                                    break;

                                case DialogGraphNodeType.Phrase:
                                    graphNode = new PhraseDialogGraphNode();
                                    break;

                                default:
                                    throw new Exception(
                                        "DialogGraph.LoadFromFile: something goes wrong. Type of node is not defined.");
                            }

                            if (graphNode is DialogGraphPhraseNodeBase)
                            {
                                var phrase = (DialogGraphPhraseNodeBase) graphNode;
                                string val;
                                if (sNode.TryGetAttributeValue("Speaker", out val))
                                {
                                    mdReader.RegisterCharacter(val);
                                    phrase.Character = val;
                                }
                                if (sNode.TryGetAttributeValue("Phrase", out val))
                                    phrase.Phrase = val;
                            }

                            graphNode.Id = _nodeIdentity++;
                            _nodes.Add(graphNode.Id, graphNode);
                            nodeMapping.Add(sNode.Name, graphNode.Id);

                            foreach (var pair in nodeMapping)
                            {
                                var defaultLink = mdReader.GetDefaultNodeLink(pair.Key);
                                int nodeId;
                                List<DialogGraphLink> links;
                                if (defaultLink != null && nodeMapping.TryGetValue(defaultLink, out nodeId) &&
                                    _links.TryGetValue(nodeId, out links) && links != null)
                                {
                                    int targetNodeId = pair.Value;
                                    var link =
                                        links.FirstOrDefault(
                                            l => l.Direction.FromNode == nodeId && l.Direction.ToNode == targetNodeId);

                                    if (link != null && CanBeDefaultLink(link))
                                    {
                                        _nodes[targetNodeId].DefaultLinkHere = link;
                                    }
                                }
                            }
                        }

                        if (sLinks != null)
                        {
                            foreach (var sLink in sLinks.GetElements<HrdArray>())
                            {
                                int from, to;
                                string str;
                                if (!sLink.TryGetArrayElement(0, out str))
                                    continue;

                                if (str == string.Empty)
                                {
                                    from = RootNode.Id;
                                }
                                else if (!nodeMapping.TryGetValue(str, out from))
                                    continue;

                                if (!sLink.TryGetArrayElement(1, out str))
                                    continue;

                                if (str == string.Empty)
                                {
                                    if (from == RootNode.Id)
                                        throw new Exception("Incorrect format of a link: from null to null.");

                                    Link(from, null);
                                    continue;
                                }

                                if (!nodeMapping.TryGetValue(str, out to))
                                    continue;

                                var link = Link(from, to);
                                if (sLink.TryGetArrayElement(2, out str) && !string.IsNullOrEmpty(str))
                                    link.Condition = str;
                                if (sLink.TryGetArrayElement(3, out str) && !string.IsNullOrEmpty(str))
                                    link.Action = str;
                            }
                        }

                        if (sStartNode != null && !string.IsNullOrEmpty(sStartNode.Value as string))
                        {
                            string nodeName = (string) sStartNode.Value;
                            int toNode;
                            if (nodeMapping.TryGetValue(nodeName, out toNode))
                            {
                                Link(RootNode.Id, toNode);
                            }
                        }

                        List<DialogGraphLink> rootLinks;
                        if (_nodes.Count > 1 &&
                            (!_links.TryGetValue(RootNode.Id, out rootLinks) || rootLinks == null ||
                             rootLinks.Count == 0))
                            throw new Exception("No dialog entry found.");
                    }
                }
            }
            finally
            {
                _raiseChanged = true;
            }
        }

        internal void SaveToFile(string filePath, IDialogUserProperties dialogUserProperties)
        {
            var sDoc = new HrdDocument();

            var sLinks = new HrdArray("Links");
            var sNodes = new HrdNode("Nodes");

            sDoc.AddElements(sNodes, sLinks);

            HashSet<int> visitedNodes = new HashSet<int>();
            var nodeQueue = new Queue<int>();

            List<DialogGraphLink> links;

            nodeQueue.Enqueue(RootNode.Id);

            while (nodeQueue.Count>0)
            {
                var currentNodeId = nodeQueue.Dequeue();
                visitedNodes.Add(currentNodeId);
                var node = _nodes[currentNodeId];

                HrdNode sNode = null;
                if (currentNodeId != RootNode.Id)
                {
                    sNode = new HrdNode("N" + currentNodeId);

                    sNode.AddElement(new HrdAttribute("Type") {Value = (int) node.NodeType});

                    var phNode = node as DialogGraphPhraseNodeBase;
                    if (phNode != null)
                    {
                        sNode.AddElements(new HrdAttribute("Speaker", phNode.Character ?? string.Empty),
                                          new HrdAttribute("Phrase", phNode.Phrase ?? string.Empty));
                    }
                }

                if(_links.TryGetValue(currentNodeId,out links) && links!=null && links.Count>0)
                {
                    foreach(var link in links)
                    {
                        var sLink = new HrdArray();

                        sLink.AddElements(new HrdAttribute(null, sNode == null ? string.Empty : sNode.Name),
                                          new HrdAttribute(null,
                                                         link.Direction.ToNode == null
                                                             ? string.Empty
                                                             : "N" + link.Direction.ToNode.Value));
                        if(!string.IsNullOrEmpty(link.Condition) || !string.IsNullOrEmpty(link.Action))
                        {
                            sLink.AddElement(new HrdAttribute(null,link.Condition??string.Empty));
                            if (!string.IsNullOrEmpty(link.Action))
                                sLink.AddElement(new HrdAttribute(null, link.Action));
                        }
                        sLinks.AddElement(sLink);

                        if (link.Direction.ToNode.HasValue && !visitedNodes.Contains(link.Direction.ToNode.Value))
                            nodeQueue.Enqueue(link.Direction.ToNode.Value);
                    }
                }

                if (sNode != null)
                {
                    if (node.DefaultLinkHere != null)
                        dialogUserProperties.SetDefaultLink("N" + node.DefaultLinkHere.Direction.FromNode, sNode.Name);

                    sNodes.AddElement(sNode);
                }
            }

            using(var fStream=new FileStream(filePath,FileMode.Create,FileAccess.Write))
            {
                sDoc.WriteDocument(fStream);
            }
        }

        private void OnGraphChanged()
        {
            var eh = GraphChanged;
            if (eh != null && _raiseChanged)
                GraphChanged(this, EventArgs.Empty);
        }

        private void OnLinkPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            OnGraphChanged();
        }

        private void OnNodePropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            OnGraphChanged();
        }
    }
}