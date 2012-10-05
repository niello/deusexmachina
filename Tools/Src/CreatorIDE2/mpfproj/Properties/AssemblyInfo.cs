using System;
using System.Diagnostics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Resources;

[assembly: AssemblyTitle("Microsoft.VisualStudio.Project")]
[assembly: AssemblyDescription("MPF Implementation of VS Projects for CreatorIDE")]
[assembly: AssemblyConfiguration("")]
[assembly: AssemblyCompany("Microsoft")]
[assembly: AssemblyProduct("Microsoft.VisualStudio.Project")]
[assembly: AssemblyCopyright("Copyright © Microsoft 2008")]
[assembly: AssemblyTrademark("")]
[assembly: AssemblyCulture("")]

[assembly: ComVisible(false)]
[assembly: CLSCompliant(false)]

[assembly: Guid("AC657363-1CF7-457d-9949-5F6E81D19A05")]

//[assembly: AssemblyVersion("1.0.0.0")]
//[assembly: AssemblyFileVersion("1.0.0.0")]

[assembly: SecurityPermissionAttribute(SecurityAction.RequestMinimum, Assertion = true)]
[assembly: IsolatedStorageFilePermissionAttribute(SecurityAction.RequestMinimum, Unrestricted = true)]
[assembly: UIPermissionAttribute(SecurityAction.RequestMinimum, Unrestricted = true)]
[assembly: PermissionSetAttribute(SecurityAction.RequestRefuse, Unrestricted = false)]
[assembly: FileIOPermissionAttribute(SecurityAction.RequestMinimum, Unrestricted = true)]
[assembly: ReflectionPermissionAttribute(SecurityAction.RequestMinimum, Unrestricted = true)]
[assembly: EventLogPermissionAttribute(SecurityAction.RequestMinimum, Unrestricted = true)]
[assembly: EnvironmentPermissionAttribute(SecurityAction.RequestMinimum, Unrestricted = true)]
[assembly: RegistryPermissionAttribute(SecurityAction.RequestRefuse)]
[assembly: NeutralResourcesLanguageAttribute("en")]
