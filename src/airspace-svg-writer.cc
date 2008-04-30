/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "exception.hh"
#include "airspace.hh"
#include "airspace-io.hh"

#include <ostream>
#include <iomanip>

#include <assert.h>

class SVGAirspaceWriter : public AirspaceWriter {
public:
    std::ostream &stream;

public:
    SVGAirspaceWriter(std::ostream *stream);

public:
    virtual void write(const Airspace &as);
    virtual void flush();
};

SVGAirspaceWriter::SVGAirspaceWriter(std::ostream *_stream)
    :stream(*_stream) {
    stream << "<?xml version=\"1.0\" standalone=\"no\"?>\n"
        "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n"
        "    \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
        "\n"
        "<svg version=\"1.1\"\n"
        "    xmlns=\"http://www.w3.org/2000/svg\">\n";
}

static void
transform(const SurfacePosition &position, int &x, int &y)
{
    x = (position.getLongitude().getValue() - 360008) / 500;
    y = (position.getLatitude().getValue() - 2800000) / 500;
}

static int
transform(const Distance &distance)
{
    return (int)(distance.toUnit(Distance::UNIT_NAUTICAL_MILES).getValue() * 1000 / 500);
}

void
SVGAirspaceWriter::write(const Airspace &as)
{
    stream << "  <g>\n";
    stream << "  <polygon points=\"";

    Airspace::EdgeList circles;

    const Airspace::EdgeList &edges = as.getEdges();
    for (Airspace::EdgeList::const_iterator it = edges.begin();
         it != edges.end(); ++it) {
        const Edge &edge = *it;

        switch (edge.getType()) {
        case Edge::TYPE_VERTEX:
            int x, y;

            transform(edge.getEnd(), x, y);

            stream << x << "," << y << " ";
            break;

        case Edge::TYPE_CIRCLE:
            circles.push_back(edge);
            break;

        case Edge::TYPE_ARC:
            break;
        }
    }

    stream << "\" style=\"fill:#cccccc; fill-opacity:0.3; stroke:#000000; stroke-width:1\"/>\n";

    for (Airspace::EdgeList::const_iterator it = circles.begin();
         it != circles.end(); ++it) {
        const Edge &edge = *it;
        assert(edge.getType() == Edge::TYPE_CIRCLE);

        int x, y;
        transform(edge.getCenter(), x, y);

        stream << "<circle cx=\"" << x << "\" cy=\"" << y
               << "\" r=\"" << transform(edge.getRadius())
               << "\" fill=\"#cccccc\" fill-opacity=\"0.3\" "
               << " stroke=\"#000000\" stroke-width=\"1\"/>\n";
    }
    stream << "  </g>\n";
}

void
SVGAirspaceWriter::flush()
{
    stream << "</svg>\n";
}

AirspaceReader *
SVGAirspaceFormat::createReader(std::istream *) const
{
    return NULL;
}

AirspaceWriter *
SVGAirspaceFormat::createWriter(std::ostream *stream) const
{
    return new SVGAirspaceWriter(stream);
}
