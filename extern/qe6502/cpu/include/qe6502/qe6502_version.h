/*
 * MIT License
 *
 * Copyright (c) 2026 Nikolay Nedelchev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#ifndef QE6502_VERSION_H
#define QE6502_VERSION_H

/*
 * Public source/API version.
 *
 * The packed value uses major in bits 16..31 and minor in bits 0..15.
 */
#define QE6502_VERSION_MAJOR 1u
#define QE6502_VERSION_MINOR 0u
#define QE6502_VERSION \
    ((QE6502_VERSION_MAJOR << 16u) | QE6502_VERSION_MINOR)

#endif /* QE6502_VERSION_H */
