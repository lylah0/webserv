/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientState.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 12:48:54 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/12 11:35:00 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENTSTATE_HPP
#define CLIENTSTATE_HPP

#include <string>

struct ClientState {
    bool headersComplete;
    bool requestReady;

    size_t contentLength;
    bool isChunked;

    // Body accumulation
    size_t bodyBytesRead;
    std::string body;

    // Chunked decoding state
    bool haveChunkSize;
    size_t currentChunkSize;
    size_t pos; // parsing cursor inside read buffer

    // Error + connection control
    bool chunkedError;
    bool closeAfterWrite;

    // Location-based limit
    size_t maxBodySize;

    ClientState()
        : headersComplete(false),
          requestReady(false),
          contentLength(0),
          isChunked(false),
          bodyBytesRead(0),
          haveChunkSize(false),
          currentChunkSize(0),
          pos(0),
          chunkedError(false),
          closeAfterWrite(false),
          maxBodySize(0)
    {}
};


#endif
