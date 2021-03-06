/*
 * wxChecksums
 * Copyright (C) 2003-2004 Julien Couot
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/**
 * \file sfvfile.cpp
 * Classes that encapsulate SFV files.
 */

//---------------------------------------------------------------------------
// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#include <wx/wx.h>
#endif

#include <wx/txtstrm.h>
#include <wx/wfstream.h>

#include "sfvfile.hpp"
#include "appprefs.hpp"
#include "crc32.hpp"
#include "osdep.hpp"
#include "utils.hpp"

#include "compat.hpp"
//---------------------------------------------------------------------------

/// The C++ standard namespace.
using namespace std;


/**
 * Default constructor.
 */
SFVFile::SFVFile() : SumFile()
{
}
//---------------------------------------------------------------------------


/**
 * Clones the source instance in this instance.
 *
 * Don't forget to call this method when cloning inherited classes.
 *
 * @param  source  Source instance.
 */
void SFVFile::clone(const SFVFile& source)
{
  SumFile::clone(source);
}
//---------------------------------------------------------------------------


/**
 * Copy constructor.
 *
 * @param  source  Source instance.
 */
SFVFile::SFVFile(const SFVFile& source)
{
  clone(source);
}
//---------------------------------------------------------------------------


/**
 * Assignment operator.
 *
 * @param  source  Source instance.
 * @return A reference on the instance.
 */
SFVFile& SFVFile::operator=(const SFVFile& source)
{
  clone(source);
  return *this;
}
//---------------------------------------------------------------------------


/**
 * Returns an instance of a class that permits to compute the CRC-32 value.
 *
 * The calling function is responsible of the deletion of the instance with
 * the <CODE>delete</CODE> operator.
 *
 * @return An instance of a class that permits to compute the checksum value.
 */
Checksum* SFVFile::getChecksumCalculator() const
{
  return new CRC32();
}
//---------------------------------------------------------------------------


/**
 * Returns the type of the file.
 *
 * @return The type of the file.
 */
wxString SFVFile::getFileType() const
{
  return wxString(wxT("SFV"));
}
//---------------------------------------------------------------------------


/**
 * Reads the checksums from a SFV file.
 *
 * After the reading of the file, the state of the file should be unmodified
 * and on success, the checksum file name must be the given one (absolute
 * path).
 *
 * The paths of the files in the ChecksumData must be relative to the path of 
 * <CODE>fileName</CODE>.
 *
 * @param  fileName  The file name from which the checksums are read.
 * @return <CODE>true</CODE> if the file has been read successfully,
 *         <CODE>false</CODE> otherwise.
 */
bool SFVFile::read(const wxFileName& fileName)
{
  const int MAX_INVALID = 64;
  wxString line;          // line of text
  size_t   l;             // size of the line
  size_t   i;             // counter
  bool     comment;       // line is a comment
  bool     stop;          // stop searching for comment
  int      invalid = 0;   // counter of invalid lines
  int      valid = 0;     // counter of valid lines
  int      ccomment = 0;  // counter of comments lines
  wxChar   c;             // a character
  wxString name;          // name of the file
  wxString sum;           // value of the checksum
  wxFileName   nameRel;   // File name with a relative path
  wxFileName   nameAbs;   // File name with ab absolute path
  wxPathFormat format;    // Format of the path separators in the file

  // No log
  wxLogNull logNo;

  // Gets the path separator
  format = static_cast<wxPathFormat>(AppPrefs::get()->readLong(prSFV_READ_PATH_SEPARATOR));
  if (format == wxPATH_NATIVE)
    format = getPathFormat(fileName);

  // Opens the file
  wxFileInputStream input(fileName.GetFullPath());
  if (!input.Ok())
    return false;
  wxTextInputStream text(input);

  // Resets the file.
  reset();
  
  // Reads the lines
  line = text.ReadLine();
  while (!input.Eof() && invalid - valid <= MAX_INVALID)
  {
    l = line.size();

    // Checks if the line is a comment or is empty
    i = 0;
    stop = false;
    comment = line.empty();
    while (!stop && !comment && i < l)
    {
      c = line[i];
      if (c == wxT(';'))
        comment = true;
      else
        if (line[i] == wxT(' ') || line[i] == wxT('\t'))
          i++;
        else
          stop = true;
    }
    
    if (comment || i == l)
    // Ignore the line, but the line is valid
    {
      valid++;
      ccomment++;
    }
    else  // Reads the file name and the checksum
    {
      name = line.BeforeLast(wxT(' '));
      sum = line.AfterLast(wxT(' '));
      if (name.empty() || !IsValidChecksum(sum))
      // Try with tab
      {
        name = line.BeforeLast(wxT('\t'));
        sum = line.AfterLast(wxT('\t'));
      }
      if (name.empty() || !IsValidChecksum(sum))
      // Line is not valid
        invalid++;
      else
      {
        valid++;

        nameAbs.Assign(name, format);
        if (!nameAbs.IsAbsolute())
          nameAbs.MakeAbsolute(fileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
        nameRel = nameAbs;
        if (!nameRel.IsRelative())
          nameRel.MakeRelativeTo(fileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));

        addChecksumData(ChecksumData(nameRel, sum, ChecksumData::NotVerified));
      }
    }
    
    line = text.ReadLine();
  }

  valid -= ccomment;
  if ((invalid - valid <= MAX_INVALID && valid > 0) || 
      (valid == 0 && invalid == 0))
  {
    setFileName(fileName.GetFullPath());
    setModified(false);
    return true;
  }

  return false;
}
//---------------------------------------------------------------------------


/**
 * Writes the checksums in a file.
 *
 * After the writing of the file, the state of the file should be unmodified
 * and the file name must be modified to <CODE>fileName</CODE>.
 * The paths of the files in the ChecksumData must be relative to the path of 
 * <CODE>fileName</CODE>.
 *
 * @param  fileName  The file name in which the checksums are written.
 * @return <CODE>true</CODE> if the checksums have been written successfully,
 *         <CODE>false</CODE> otherwise.
 */
bool SFVFile::write(const wxFileName& fileName)
{
  wxLogNull      logNo;   // No log
  wxString       line;    // line of text
  wxDateTime     d;       // A date
  wxFileName     nameRel; // File name with a relative path
  wxFileName     nameAbs; // File name with ab absolute path
  wxCOff_t       length;  // Length of the file
  wxPathFormat   format;  // Format of the path separators in the file
  
  wxFileOutputStream output(fileName.GetFullPath());
  if (!output.Ok())
    return false;
  wxTextOutputStream text(output, static_cast<wxEOL>(AppPrefs::get()->readLong(prSFV_WRITE_EOL)));

  // Gets the path separator
  format = static_cast<wxPathFormat>(AppPrefs::get()->readLong(prSFV_WRITE_PATH_SEPARATOR));

  // Write header
  if (AppPrefs::get()->readBool(prSFV_WRITE_GEN_AND_DATE))
  {
    d = wxDateTime::Now();
    wxString generator = AppPrefs::get()->readString(prSFV_IDENTIFY_AS);
    if (generator.IsEmpty())
      generator = ::getAppName();
    line.Printf(wxT("; Generated by %s on %s at %s\n"), generator.c_str(),
                d.Format(wxT("%Y-%m-%d")).c_str(), d.Format(wxT("%H:%M:%S")).c_str());
    text.WriteString(line);
    text.WriteString(wxT(";\n"));
  }
  
  // Write the size and the date of the files
  if (AppPrefs::get()->readBool(prSFV_WRITE_FILE_SIZE_AND_DATE))
  {
    MChecksumData::const_iterator it = getChecksumDataBegin();
    MChecksumData::const_iterator end = getChecksumDataEnd();
    
    while (it != end)
    {
      const ChecksumData& cd = it->second;
      nameAbs = cd.getFileName();
      if (!nameAbs.IsAbsolute())
        nameAbs.MakeAbsolute(wxFileName(this->getFileName()).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
      nameRel = nameAbs;
      if (!nameRel.IsRelative())
        nameRel.MakeRelativeTo(fileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));

      if ((length = wxCGetFileLength(nameAbs.GetFullPath())) != static_cast<wxCOff_t>(wxInvalidOffset))
      {
        #if defined(wxC_USE_LARGE_FILES)
        text.WriteString(wxString::Format(wxT("; %12" wxLongLongFmtSpec "u  "), length));
        #else
        text.WriteString(wxString::Format(wxT("; %12u  "), length));
        #endif
        d = nameAbs.GetModificationTime();
        text << d.Format(wxT("%H:%M.%S")) << wxT(" ") << d.Format(wxT("%Y-%m-%d"));
        text << wxT(" ") << nameRel.GetFullPath(format) << wxT("\n");
      }
      it++;
    }
  }
  
  // Write checksums
  MChecksumData::const_iterator it = getChecksumDataBegin();
  MChecksumData::const_iterator end = getChecksumDataEnd();
    
  while (it != end)
  {
    const ChecksumData& cd = it->second;
    nameAbs = cd.getFileName();
    if (!nameAbs.IsAbsolute())
      nameAbs.MakeAbsolute(wxFileName(this->getFileName()).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
    nameRel = nameAbs;
    if (!nameRel.IsRelative())
      nameRel.MakeRelativeTo(fileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));

    text << nameRel.GetFullPath(format) << wxT(" ")
         << cd.getChecksum().Upper() << wxT("\n");
    it++;
  }
  
  if (output.IsOk())
  {
    // Modify file paths if the path of the checksum file has been modified.
    {
      MChecksumData::iterator it = getChecksumDataBeginI();
      MChecksumData::iterator end = getChecksumDataEndI();

      while (it != end)
      {
        ChecksumData& cd = it->second;
        nameAbs = cd.getFileName();
        if (!nameAbs.IsAbsolute())
          nameAbs.MakeAbsolute(wxFileName(this->getFileName()).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
        nameRel = nameAbs;
        if (!nameRel.IsRelative())
          nameRel.MakeRelativeTo(fileName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));

        cd.setFileName(nameRel);
        it++;
      }
    }
    
    this->setFileName(fileName.GetFullPath());
    this->setModified(false);
    return true;
  }
  else
    return false;
}
//---------------------------------------------------------------------------


/**
 * Indicates if the given checksum is valid.
 *
 * @param  checksum  The checksum to check.
 */
bool SFVFile::IsValidChecksum(const wxString& checksum)
{
  const wxString validCaracters = wxT("0123456789abcdefABCDEF");
  wxChar c;
  size_t i, j;
  size_t l = checksum.size();
  size_t s = validCaracters.size();
  bool   OK, found;

  i = 0;
  OK = (checksum.size() == 8);
  while (OK && i < l)
  {
    c = checksum[i];
    j = 0;
    found = false;
    while (!found && j < s)
    {
      if (c == validCaracters[j])
        found = true;
      j++;
    }
    
    if (!found)
      OK = false;
    i++;
  }
  
  return OK;
}
//---------------------------------------------------------------------------


/**
 * Gets a new instance of this class.
 *
 * The caller is responsible of the deletion of the instance with the
 * <CODE>delete</CODE> operator.
 *
 * @return A new instance of this class.
 */
SumFile* SFVFile::getNewInstance()
{
  return new SFVFile();
}
//---------------------------------------------------------------------------
