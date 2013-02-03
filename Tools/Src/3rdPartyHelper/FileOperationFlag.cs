using System;

namespace ThirdPartyHelper
{
    [Flags]
    public enum FileOperationFlag
    {
        /// <summary>
        /// Do not display a progress dialog box.  
        /// </summary>
        Silent = 4,
        /// <summary>
        /// Give the file being operated on a new name in a move, copy, or rename operation if a file with the target name already exists.  
        /// </summary>
        RenameOnCollision = 8,
        /// <summary>
        /// Respond with "Yes to All" for any dialog box that is displayed.  
        /// </summary>
        NoConfirmation = 16,
        /// <summary>
        /// Preserve undo information, if possible. 
        /// </summary>
        AllowUndo = 64,
        /// <summary>
        /// Perform the operation on files only if a wildcard file name (*.*) is specified. 
        /// </summary>
        FilesOnly = 128,
        /// <summary>
        /// Display a progress dialog box but do not show the file names.  
        /// </summary>
        SimpleProgress = 256,
        /// <summary>
        /// Do not confirm the creation of a new directory if the operation requires one to be created.
        /// </summary>
        NoConfirmMkDir = 512,
        /// <summary>
        /// Do not display a user interface if an error occurs.  
        /// </summary>
        NoErrorUI = 1024,
        /// <summary>
        /// Version 4.71. Do not copy the security attributes of the file. 
        /// </summary>
        NoCopyUserSecurityAttribs = 2048,
        /// <summary>
        /// Only operate in the local directory. Don't operate recursively into subdirectories. 
        /// </summary>
        NoRecursion = 4096,
        /// <summary>
        /// Version 5.0. Do not copy connected files as a group. Only copy the specified files.
        /// </summary>
        NoConnectedElements = 9182,
    }
}
