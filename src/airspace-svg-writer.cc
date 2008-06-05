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

static const char *
airspace_style(const Airspace &airspace)
{
    switch (airspace.getType()) {
    case Airspace::TYPE_CHARLY:
    case Airspace::TYPE_DELTA:
        return "fill:#00d000; fill-opacity:0.2; stroke:#008000; stroke-width:1";

    case Airspace::TYPE_CTR:
        return "fill:#d00000; fill-opacity:0.3; stroke:#800000; stroke-width:1";

    case Airspace::TYPE_FOX:
        return "fill:#00d0a0; fill-opacity:0.3; stroke:#008060; stroke-width:1";

    default:
        return "fill:#cccccc; fill-opacity:0.3; stroke:#000000; stroke-width:1";
    }
}

static int
transform(const Latitude &latitude)
{
    return (latitude.getValue() - 2800000) / 500;
}

static int
transform(const Longitude &longitude)
{
    return (longitude.getValue() - 360008) / 500;
}

static int
transform(const Distance &distance)
{
    return (int)(distance.toUnit(Distance::UNIT_NAUTICAL_MILES).getValue() * 1000 / 500);
}

static std::ostream &
operator <<(std::ostream &os, const Latitude &latitude)
{
    return os << transform(latitude);
}

static std::ostream &
operator <<(std::ostream &os, const Longitude &longitude)
{
    return os << transform(longitude);
}

static std::ostream &
operator <<(std::ostream &os, const SurfacePosition &position)
{
    return os << position.getLongitude() << ","
              << position.getLatitude();
}

static std::ostream &
operator <<(std::ostream &os, const Distance &distance)
{
    return os << transform(distance);
}

void
SVGAirspaceWriter::write(const Airspace &as)
{
    stream << "  <g>\n";
    stream << "  <path d=\"";

    Airspace::EdgeList circles;

    const Airspace::EdgeList &edges = as.getEdges();
    for (Airspace::EdgeList::const_iterator it = edges.begin();
         it != edges.end(); ++it) {
        const Edge &edge = *it;

        switch (edge.getType()) {
        case Edge::TYPE_VERTEX:
            if (it == edges.begin())
                stream << "M";
            else
                stream << "L";

            stream << edge.getEnd() << " ";
            break;

        case Edge::TYPE_CIRCLE:
            circles.push_back(edge);
            break;

        case Edge::TYPE_ARC:
            const Distance radius = edge.getEnd() - edge.getCenter();

            stream << "A" << radius << "," << radius
                   << " 0 0,0 " << edge.getEnd() << " ";
            break;
        }
    }

    stream << "Z\" style=\"" << airspace_style(as) << "\"/>\n";

    for (Airspace::EdgeList::const_iterator it = circles.begin();
         it != circles.end(); ++it) {
        const Edge &edge = *it;
        assert(edge.getType() == Edge::TYPE_CIRCLE);

        stream << "<circle cx=\"" << edge.getCenter().getLongitude()
               << "\" cy=\"" << edge.getCenter().getLatitude()
               << "\" r=\"" << transform(edge.getRadius())
               << "\" style=\"" << airspace_style(as) << "\"/>\n";
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
