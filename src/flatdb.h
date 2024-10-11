// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FLATDB_H
#define BITCOIN_FLATDB_H

#include <fs.h>

#include <string>

template <typename T>
class CFlatDB
{
private:
    enum ReadResult {
        Ok,
        FileError,
        HashReadError,
        IncorrectHash,
        IncorrectMagicMessage,
        IncorrectMagicNumber,
        IncorrectFormat
    };

    fs::path pathDB;
    std::string strFilename;
    std::string strMagicMessage;

    bool CoreWrite(const T& objToSave);
    ReadResult CoreRead(T& objToLoad);

    bool Read(T& objToLoad);

public:
    CFlatDB(std::string&& strFilenameIn, std::string&& strMagicMessageIn);
    ~CFlatDB();

    bool Load(T& objToLoad);
    bool Store(const T& objToSave);
};

#endif // BITCOIN_FLATDB_H
