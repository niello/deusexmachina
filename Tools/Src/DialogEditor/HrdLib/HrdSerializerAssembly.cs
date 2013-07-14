using System;
using System.CodeDom.Compiler;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using Microsoft.CSharp;

namespace HrdLib
{
    internal partial class HrdSerializerAssembly
    {
        private const string
            AssemblyNamespace = "HrdLib.GeneratedAssembly",
            ClassName = "HrdExtendedSerializer";

        private readonly object _syncObject = new object();
        private readonly Type _type;

        private volatile Type _generatedSerializerType;

        public HrdSerializerAssembly(Type type)
        {
            Debug.Assert(type != null);
            _type = type;
        }

        public HrdSerializer CreateSerializer()
        {
            if (_generatedSerializerType == null)
            {
                lock (_syncObject)
                {
                    if (_generatedSerializerType == null)
                        _generatedSerializerType = GenerateSerializer();
                }
            }
            var serializer = Activator.CreateInstance(_generatedSerializerType);
            return (HrdSerializer) serializer;
        }

        private Type GenerateSerializer()
        {
            HrdIndentWriter writer = null;
            try
            {
                var compilerParameters = new CompilerParameters();

                if (HrdLidDiagnostics.KeepFiles)
                {
                    var fName = Path.GetFileNameWithoutExtension(Path.GetTempFileName());
                    Debug.Assert(!string.IsNullOrEmpty(fName));

                    if (!Directory.Exists(HrdLidDiagnostics.AssemblyGeneratorFolder))
                        Directory.CreateDirectory(HrdLidDiagnostics.AssemblyGeneratorFolder);
                    var subDir = Path.Combine(HrdLidDiagnostics.AssemblyGeneratorFolder, fName);
                    if (!Directory.Exists(subDir))
                        Directory.CreateDirectory(subDir);

                    // Be carefull! Compiler will keep all files in the filesystem.
                    compilerParameters.TempFiles = new TempFileCollection(subDir, true);
                    compilerParameters.IncludeDebugInformation = true;
                    compilerParameters.OutputAssembly = Path.Combine(subDir, AssemblyNamespace + HrdLidDiagnostics.AssemblyFileExtension);
                    
                    var fullPath = Path.Combine(HrdLidDiagnostics.AssemblyGeneratorFolder, fName + HrdLidDiagnostics.TempFileExtension);
                    writer = new HrdIndentWriter(fullPath, ' ', 4);
                }
                else
                {
                    writer = new HrdIndentWriter(' ', 4);
                }

                GenerateCode(compilerParameters, writer);

                CompilerResults codeGenResult;
                using (var codeProvider = new CSharpCodeProvider())
                    codeGenResult = codeProvider.CompileAssemblyFromSource(compilerParameters, writer.GetCode());

                if (codeGenResult.Errors != null && codeGenResult.Errors.Count > 0 && codeGenResult.Errors.HasErrors)
                    throw new HrdCompilerException(SR.GetString(SR.InternalCodeGenError), codeGenResult.Errors);

                var result = codeGenResult.CompiledAssembly.GetType(AssemblyNamespace + "." + ClassName, true);
                return result;
            }
            finally
            {
                if (writer != null)
                    writer.Dispose();
            }
        }

        private void GenerateCode(CompilerParameters compilerParameters, HrdIndentWriter writer)
        {
            writer.WriteLine("namespace {0}", AssemblyNamespace).WriteLine("{");
            writer.IncreaseIndent();

            var baseType = typeof (HrdSerializer);
            writer.WriteLine("public class {0}: {1}", ClassName, ReflectionHelper.GetCsTypeName(baseType));

            var queue = new HrdGeneratorQueue(_type);
            var objType = typeof(object);
            var writerType = typeof(HrdWriter);
            queue.AddType(objType, false);
            queue.AddType(writerType, false);

            var serializerInterface = typeof (IHrdSerializer<>);
            writer.IncreaseIndent();
            foreach (var rootType in queue.GetRootTypes())
            {
                var genericSerializerType = serializerInterface.MakeGenericType(rootType);
                writer.WriteLine(", {0}", ReflectionHelper.GetCsTypeName(genericSerializerType));
            }
            writer.DecreaseIndent();

            writer.Write("{").IncreaseIndent();

            

            writer.WriteLine().WriteLine("public {0}():", ClassName).IncreaseIndent();
            writer.WriteLine("base(typeof({0}))", ReflectionHelper.GetCsTypeName(_type)).DecreaseIndent();
            writer.WriteLine("{").WriteLine("}");
   
            while(queue.MoveNext())
            {
                writer.WriteLine().Write(queue.IsRoot ? "public" : "private").Write(" ")
                      .WriteLine("void Serialize({0} writer, {1} value)", ReflectionHelper.GetCsTypeName<HrdWriter>(),
                                 ReflectionHelper.GetCsTypeName(queue.CurrentType)).WriteLine("{").IncreaseIndent();

                BuildTypeSerializationMethod(writer, queue);

                writer.DecreaseIndent();
                writer.WriteLine("}");

                writer.WriteLine()
                      .WriteLine("private {0} Deserialize{1}({2} reader)",
                                 ReflectionHelper.GetCsTypeName(queue.CurrentType), queue.CurrentTypeID,
                                 ReflectionHelper.GetCsTypeName<HrdReader>())
                      .WriteLine("{")
                      .IncreaseIndent();

                BuildTypeDeserializationMethod(writer, queue);

                writer.DecreaseIndent();
                writer.WriteLine("}");

                if (queue.IsRoot)
                {
                    var genericSerializerInterface = serializerInterface.MakeGenericType(queue.CurrentType);
                    writer.WriteLine()
                          .WriteLine("{0} {1}.Deserialize({2} reader)",
                                     ReflectionHelper.GetCsTypeName(queue.CurrentType),
                                     ReflectionHelper.GetCsTypeName(genericSerializerInterface),
                                     ReflectionHelper.GetCsTypeName<HrdReader>())
                          .WriteLine("{")
                          .IncreaseIndent();

                    writer.WriteLine("return Deserialize{0}(reader);", queue.CurrentTypeID).DecreaseIndent();

                    writer.WriteLine("}");
                }
            }

            writer.WriteLine()
                  .WriteLine("protected override void Serialize({1} writer, {0} obj)", ReflectionHelper.GetCsTypeName(objType),
                             ReflectionHelper.GetCsTypeName(writerType)).WriteLine("{").IncreaseIndent();
            writer.WriteLine("Serialize(writer, ({0}) obj);", ReflectionHelper.GetCsTypeName(_type)).DecreaseIndent();
            writer.WriteLine("}");

            writer.WriteLine()
                  .WriteLine("protected override {0} Deserialize({1} reader)", ReflectionHelper.GetCsTypeName(objType),
                             ReflectionHelper.GetCsTypeName<HrdReader>())
                  .WriteLine("{")
                  .IncreaseIndent();

            writer.WriteLine("return Deserialize{0}(reader);", queue.AddType(_type)).DecreaseIndent();
            writer.WriteLine("}");

            // class
            writer.DecreaseIndent();
            writer.WriteLine("}");

            // namespace
            writer.DecreaseIndent();
            writer.Write("}");

            writer.Flush();

            var assemblies = queue.GetReferencedAssemblies();
            foreach (var asmName in assemblies)
            {
                var asm = Assembly.Load(asmName);
                compilerParameters.ReferencedAssemblies.Add(asm.Location);
            }
        }

        private static HrdSerializableAttribute GetSerializableAttribute(Type type)
        {
            var serializableAttrs = type.GetCustomAttributes(typeof(HrdSerializableAttribute), false);
            HrdSerializableAttribute serizalizableAttribute;
            if (serializableAttrs.Length == 0)
            {
                // Class is not marked for HRD serialization directly. But it could be serialized with the default parameters.
                serizalizableAttribute = new HrdSerializableAttribute();
            }
            else
            {
                Debug.Assert(serializableAttrs.Length == 1, "Multiple attributes are not allowed.");
                serizalizableAttribute = ((HrdSerializableAttribute)serializableAttrs[0]).Clone();
            }

            return serizalizableAttribute;
        }

        private static string MakeVerbatimString(string value)
        {
            if (value == null)
                return "null";

            var result = string.Concat("@\"", value.Replace("\"", "\"\""), "\"");
            return result;
        }

        private static string MakeVerbatimResourceString(string name)
        {
            return MakeVerbatimString(SR.GetString(name));
        }

        private static string MakeVerbatimResourceString(string name, params object[] args)
        {
            if (args == null || args.Length == 0)
                return MakeVerbatimResourceString(name);

            return MakeVerbatimString(SR.GetFormatString(name, args));
        }
    }
}
