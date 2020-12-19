/*
 * FLAC decoder module for K3b.
 * Based on the Ogg Vorbis module for same.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2003-2004 John Steele Scott <toojays@toojays.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bflacdecoder.h"
#include "k3bplugin_i18n.h"
#include <config-kylinburner.h>
#include <config-flac.h>

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QStringList>

#include <string.h>
#include <math.h>
#include <FLAC++/metadata.h>
#include <FLAC++/decoder.h>

K3B_EXPORT_PLUGIN(k3bflacdecoder, K3bFLACDecoderFactory)

#ifdef ENABLE_TAGLIB
#include <tag.h>
#include <flacfile.h>
#endif

#if !defined FLACPP_API_VERSION_CURRENT || FLACPP_API_VERSION_CURRENT < 6
#define LEGACY_FLAC
#else
#undef LEGACY_FLAC
#endif


class K3bFLACDecoder::Private
#ifdef LEGACY_FLAC
    : public FLAC::Decoder::SeekableStream
#else
        : public FLAC::Decoder::Stream
#endif
{
public:
    void open(QFile* f) {
        file = f;
        file->open(QIODevice::ReadOnly);

        //TODO port me to kde4
        //internalBuffer->flush();

        set_metadata_respond(FLAC__METADATA_TYPE_STREAMINFO);
        set_metadata_respond(FLAC__METADATA_TYPE_VORBIS_COMMENT);

        init();
        process_until_end_of_metadata();
    }

    void cleanup() {
        file->close();
        finish();
        delete comments;
        comments = 0;
    }

    Private(QFile* f)
#ifdef LEGACY_FLAC
        : FLAC::Decoder::SeekableStream(),
#else
          : FLAC::Decoder::Stream(),
#endif
            comments(0) {
            internalBuffer = new QBuffer();
            internalBuffer->open(QIODevice::ReadWrite);

            open(f);
        }


    ~Private() override {
        cleanup();
        delete internalBuffer;
    }

    bool seekToFrame(int frame);

    QFile* file;
    QBuffer* internalBuffer;
    FLAC::Metadata::VorbisComment* comments;
    unsigned rate;
    unsigned channels;
    unsigned bitsPerSample;
    unsigned maxFramesize;
    unsigned maxBlocksize;
    unsigned minFramesize;
    unsigned minBlocksize;
    FLAC__uint64 samples;

protected:
#ifdef LEGACY_FLAC
    virtual FLAC__SeekableStreamDecoderReadStatus read_callback(FLAC__byte buffer[], unsigned *bytes);
    virtual FLAC__SeekableStreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset);
    virtual FLAC__SeekableStreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset);
    virtual FLAC__SeekableStreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length);
#else
    FLAC__StreamDecoderReadStatus read_callback(FLAC__byte buffer[], size_t *bytes) override;
    FLAC__StreamDecoderSeekStatus seek_callback(FLAC__uint64 absolute_byte_offset) override;
    FLAC__StreamDecoderTellStatus tell_callback(FLAC__uint64 *absolute_byte_offset) override;
    FLAC__StreamDecoderLengthStatus length_callback(FLAC__uint64 *stream_length) override;
#endif
    bool eof_callback() override;
    void error_callback(FLAC__StreamDecoderErrorStatus) override{};
    void metadata_callback(const ::FLAC__StreamMetadata *metadata) override;
    ::FLAC__StreamDecoderWriteStatus write_callback(const ::FLAC__Frame *frame, const FLAC__int32 * const buffer[]) override;
};

bool K3bFLACDecoder::Private::seekToFrame(int frame) {
    FLAC__uint64 sample = static_cast<FLAC__uint64>(frame) * rate / 75;
    return seek_absolute(sample);
}

bool K3bFLACDecoder::Private::eof_callback() {
    return file->atEnd();
}

#ifdef LEGACY_FLAC
FLAC__SeekableStreamDecoderReadStatus K3bFLACDecoder::Private::read_callback(FLAC__byte buffer[],                                                                             unsigned *bytes) {
    long retval =  file->read((char *)buffer, (*bytes));
    if(-1 == retval) {
        return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_ERROR;
    } else {
        (*bytes) = retval;
        return FLAC__SEEKABLE_STREAM_DECODER_READ_STATUS_OK;
    }
}
#else
FLAC__StreamDecoderReadStatus K3bFLACDecoder::Private::read_callback(FLAC__byte buffer[],                                                                             size_t *bytes) {
    long retval =  file->read((char *)buffer, (*bytes));
    if(-1 == retval) {
        return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
    } else {
        (*bytes) = retval;
        return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
    }
}
#endif

#ifdef LEGACY_FLAC
FLAC__SeekableStreamDecoderSeekStatus
K3bFLACDecoder::Private::seek_callback(FLAC__uint64 absolute_byte_offset) {
    if(!file->seek(absolute_byte_offset))
        return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_ERROR;
    else
        return FLAC__SEEKABLE_STREAM_DECODER_SEEK_STATUS_OK;
}
#else
FLAC__StreamDecoderSeekStatus
K3bFLACDecoder::Private::seek_callback(FLAC__uint64 absolute_byte_offset) {
    if(file->seek(absolute_byte_offset) == false)
        return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
    else
        return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}
#endif

#ifdef LEGACY_FLAC
FLAC__SeekableStreamDecoderTellStatus
K3bFLACDecoder::Private::tell_callback(FLAC__uint64 *absolute_byte_offset) {
    (*absolute_byte_offset) = file->pos();
    return FLAC__SEEKABLE_STREAM_DECODER_TELL_STATUS_OK;
}
#else
FLAC__StreamDecoderTellStatus
K3bFLACDecoder::Private::tell_callback(FLAC__uint64 *absolute_byte_offset) {
    (*absolute_byte_offset) = file->pos();
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}
#endif

#ifdef LEGACY_FLAC
FLAC__SeekableStreamDecoderLengthStatus
K3bFLACDecoder::Private::length_callback(FLAC__uint64 *stream_length) {
    (*stream_length) = file->size();
    return FLAC__SEEKABLE_STREAM_DECODER_LENGTH_STATUS_OK;
}
#else
FLAC__StreamDecoderLengthStatus
K3bFLACDecoder::Private::length_callback(FLAC__uint64 *stream_length) {
    (*stream_length) = file->size();
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}
#endif


void K3bFLACDecoder::Private::metadata_callback(const FLAC__StreamMetadata *metadata) {
    switch (metadata->type) {
    case FLAC__METADATA_TYPE_STREAMINFO:
        channels = metadata->data.stream_info.channels;
        rate = metadata->data.stream_info.sample_rate;
        bitsPerSample = metadata->data.stream_info.bits_per_sample;
        samples = metadata->data.stream_info.total_samples;
        maxFramesize = metadata->data.stream_info.max_framesize;
        minFramesize = metadata->data.stream_info.min_framesize;
        maxBlocksize = metadata->data.stream_info.max_blocksize;
        minBlocksize = metadata->data.stream_info.min_blocksize;
        break;
    case FLAC__METADATA_TYPE_VORBIS_COMMENT:
        comments = new FLAC::Metadata::VorbisComment((FLAC__StreamMetadata *)metadata, true);
        break;
    default:
        break;
    }
}

FLAC__StreamDecoderWriteStatus K3bFLACDecoder::Private::write_callback(const FLAC__Frame *frame, const FLAC__int32 * const buffer[]) {
    unsigned i, j;
    // Note that in canDecode we made sure that the input is 1-16 bit stereo or mono.
    unsigned samples = frame->header.blocksize;

    for(i=0; i < samples; ++i) {
        // in FLAC channel 0 is left, 1 is right
        for(j=0; j < this->channels; ++j) {
            FLAC__int32 value = (buffer[j][i])<<(16 - frame->header.bits_per_sample);
            internalBuffer->putChar(value >> 8); // msb
            internalBuffer->putChar(value & 0xFF); // lsb
        }
    }

    // Rewind the buffer so the decode method will take data from the beginning.
    internalBuffer->seek(0);
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

K3bFLACDecoder::K3bFLACDecoder( QObject* parent  )
    : K3b::AudioDecoder( parent)
{
    d = 0;
}


K3bFLACDecoder::~K3bFLACDecoder()
{
    delete d;
}

void K3bFLACDecoder::cleanup()
{
    if (d) {
        d->cleanup();
        d->open(new QFile(filename()));
    }
    else
        d = new Private(new QFile(filename()));
}

bool K3bFLACDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& ch )
{
    cleanup();

    frames = (unsigned long)ceil((d->samples * 75.0)/d->rate);
    samplerate = d->rate;
    ch = d->channels;

    // add meta info
    if( d->comments != 0 ) {
        qDebug() << "(K3bFLACDecoder) unpacking Vorbis tags";
        for( unsigned int i = 0; i < d->comments->get_num_comments(); ++i ) {
            QString key = QString::fromUtf8( d->comments->get_comment(i).get_field_name(),
                                             d->comments->get_comment(i).get_field_name_length() );
            QString value = QString::fromUtf8( d->comments->get_comment(i).get_field_value(),
                                               d->comments->get_comment(i).get_field_value_length() );

            if( key.toUpper() == "TITLE" )
                addMetaInfo( META_TITLE, value );
            else if( key.toUpper() == "ARTIST" )
                addMetaInfo( META_ARTIST, value );
            else if( key.toUpper() == "DESCRIPTION" )
                addMetaInfo( META_COMMENT, value );
        }
    }
#ifdef ENABLE_TAGLIB
    if ((d->comments == 0) || (d->comments->get_num_comments() == 0)) {
        // no Vorbis comments, check for ID3 tags
        qDebug() << "(K3bFLACDecoder) using taglib to read tag";
        TagLib::FLAC::File f( QFile::encodeName(filename()) );
        if( f.isOpen() ) {
            addMetaInfo( META_TITLE, TStringToQString( f.tag()->title() ) );
            addMetaInfo( META_ARTIST, TStringToQString( f.tag()->artist() ) );
            addMetaInfo( META_COMMENT, TStringToQString( f.tag()->comment() ) );
        }
    }
#endif

    return true;
}


bool K3bFLACDecoder::initDecoderInternal()
{
    cleanup();

    return true;
}


int K3bFLACDecoder::decodeInternal( char* _data, int maxLen )
{
    int bytesToCopy;
    int bytesCopied;
    int bytesAvailable;

#ifdef LEGACY_FLAC
    if(d->internalBuffer->size() == 0) {
        // want more data
        switch(d->get_state()) {
        case FLAC__SEEKABLE_STREAM_DECODER_END_OF_STREAM:
            d->finish();
            break;
        case FLAC__SEEKABLE_STREAM_DECODER_OK:
            if(! d->process_single())
                return -1;
            break;
        default:
            return -1;
        }
    }
#else
    if(d->internalBuffer->size() == 0) {
        // want more data
        if(d->get_state() == FLAC__STREAM_DECODER_END_OF_STREAM)
            d->finish();
        else if(d->get_state() < FLAC__STREAM_DECODER_END_OF_STREAM) {
            if(! d->process_single())
                return -1;
        }
        else
            return -1;
    }
#endif

    bytesAvailable = d->internalBuffer->size() - d->internalBuffer->pos();
    bytesToCopy = qMin(maxLen, bytesAvailable);
    bytesCopied = (int)d->internalBuffer->read(_data, bytesToCopy);

    if(bytesCopied == bytesAvailable) {
        // reset the buffer
        d->internalBuffer->close();
        d->internalBuffer->open(QIODevice::ReadWrite|QIODevice::Truncate);
    }

    return bytesCopied;
}


bool K3bFLACDecoder::seekInternal( const K3b::Msf& pos )
{
    return d->seekToFrame(pos.totalFrames());
}


QString K3bFLACDecoder::fileType() const
{
    return i18n("FLAC");
}


QStringList K3bFLACDecoder::supportedTechnicalInfos() const
{
    return QString( i18n("Channels") + ';' +
                    i18n("Sampling Rate") + ';' +
                    i18n("Sample Size") ).split( ';' );
}


QString K3bFLACDecoder::technicalInfo( const QString& info ) const
{
    if( d->comments != 0 ) {
        if( info == i18n("Vendor") )
#ifdef FLAC_NEWER_THAN_1_1_1
            return QString::fromUtf8((char*)d->comments->get_vendor_string());
#else
        return QString::fromUtf8(d->comments->get_vendor_string().get_field());
#endif
        else if( info == i18n("Channels") )
            return QString::number(d->channels);
        else if( info == i18n("Sampling Rate") )
            return i18n("%1 Hz",d->rate);
        else if( info == i18n("Sample Size") )
            return i18np("1 bit","%1 bits",d->bitsPerSample);
    }

    return QString();
}



K3bFLACDecoderFactory::K3bFLACDecoderFactory( QObject* parent, const QVariantList& )
    : K3b::AudioDecoderFactory( parent )
{
}


K3bFLACDecoderFactory::~K3bFLACDecoderFactory()
{
}


K3b::AudioDecoder* K3bFLACDecoderFactory::createDecoder( QObject* parent) const
{
    return new K3bFLACDecoder( parent );
}


bool K3bFLACDecoderFactory::canDecode( const QUrl& url )
{
    // buffer large enough to read an ID3 tag header
    char buf[10];

    // Note: since file is created on the stack it will be closed automatically
    // by its destructor when this method (i.e. canDecode) returns.
    QFile file(url.toLocalFile());

    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "(K3bFLACDecoder) Could not open file " << url.toLocalFile();
        return false;
    }

    // look for a fLaC magic number or ID3 tag header
    if(10 != file.read(buf, 10)) {
        qDebug() << "(K3bFLACDecorder) File " << url.toLocalFile()
                 << " is too small to be a FLAC file" << endl;
        return false;
    }

    if(0 == memcmp(buf, "ID3", 3)) {
        // Found ID3 tag, try and seek past it.
        qDebug() << "(K3bFLACDecorder) File " << url.toLocalFile() << ": found ID3 tag";

        // See www.id3.org for details of the header, note that the size field
        // unpacks to 7-bit bytes, then the +10 is for the header itself.
        int pos;
        pos = ((buf[6]<<21)|(buf[7]<<14)|(buf[8]<<7)|buf[9]) + 10;

        qDebug() << "(K3bFLACDecoder) " << url.toLocalFile() << ": seeking to "
                 << pos << endl;
        if(!file.seek(pos)) {
            qDebug() << "(K3bFLACDecoder) " << url.toLocalFile() << ": couldn't seek to "
                     << pos << endl;
            return false;
        }else{
            // seek was okay, try and read magic number into buf
            if(4 != file.read(buf, 4)) {
                qDebug() << "(K3bFLACDecorder) File " << url.toLocalFile()
                         << " has ID3 tag but naught else!" << endl;
                return false;
            }
        }
    }

    if(memcmp(buf, "fLaC", 4) != 0) {
        qDebug() << "(K3bFLACDecoder) " << url.toLocalFile() << ": not a FLAC file";
        return false;
    }

    FLAC::Metadata::StreamInfo info = FLAC::Metadata::StreamInfo();
    FLAC::Metadata::get_streaminfo(url.toLocalFile().toLatin1(), info);

    if((info.get_channels() <= 2) &&
       (info.get_bits_per_sample() <= 16)) {
        return true;
    } else {
        qDebug() << "(K3bFLACDecoder) " << url.toLocalFile() << ": wrong format:" << endl
                 << "                channels:    "
                 << QString::number(info.get_channels()) << endl
                 << "                samplerate:  "
                 << QString::number(info.get_sample_rate()) << endl
                 << "                bits/sample: "
                 << QString::number(info.get_bits_per_sample()) << endl;
        return false;
    }
}

#include "k3bflacdecoder.moc"
