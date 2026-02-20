#!/usr/bin/env -S uv run
#
# Copyright (C) 2014-2026 V-Nova International Limited
#
#     * All rights reserved.
#     * This software is licensed under the BSD-3-Clause-Clear License.
#     * No patent licenses are granted under this license. For enquiries about patent
#       licenses, please contact legal@v-nova.com.
#     * The software is a stand-alone project and is NOT A CONTRIBUTION to any other
#       project.
#     * If the software is incorporated into another project, THE TERMS OF THE
#       BSD-3-CLAUSE-CLEAR LICENSE AND THE ADDITIONAL LICENSING INFORMATION CONTAINED
#       IN THIS FILE MUST BE MAINTAINED, AND THE SOFTWARE DOES NOT AND MUST NOT ADOPT
#       THE LICENSE OF THE INCORPORATING PROJECT. However, the software may be
#       incorporated into a project under a compatible license provided the
#       requirements of the BSD-3-Clause-Clear license are respected, and V-Nova
#       International Limited remains licensor of the software ONLY UNDER the
#       BSD-3-Clause-Clear license (not the compatible license).
#
# ANY ONWARD DISTRIBUTION, WHETHER STAND-ALONE OR AS PART OF ANY OTHER PROJECT,
# REMAINS SUBJECT TO THE EXCLUSION OF PATENT LICENSES PROVISION OF THE
# BSD-3-CLAUSE-CLEAR LICENSE.
#
"""Plot the output of the SIMULATE_BUFFER algorithm from a JSON file."""

from __future__ import annotations

import json
import pathlib

import plotly.express as px
import plotly.graph_objects as go
import typer

app = typer.Typer(help="Plot the output of the receive network buffer model from a JSON file.")

COLORS = px.colors.qualitative.Plotly
HOVER_TEMPLATE = "Time: %{x}<br>Fill: %{y}"


def add_line(fig: go.Figure, data: dict, target: str, title: str, col_idx: int) -> int:
    if target in data and len(data[target]["plotValues"]) > 0:
        color = COLORS[col_idx % len(COLORS)]
        col_idx += 1
        times = data[target]["plotTimes"]
        values = data[target]["plotValues"]
        fig.add_scatter(
            x=times,
            y=values,
            mode="lines",
            name=title,
            line_color=color,
            hovertemplate=HOVER_TEMPLATE,
        )
    return col_idx


@app.command()
def plot(
    files: list[pathlib.Path] = typer.Argument(
        help="Path to SIMULATE_BUFFER output JSON.",
        file_okay=True,
        dir_okay=False,
    ),
    title: str | None = typer.Option(help="Graph title", default=None),
) -> None:
    """Plot the output of the buffer model from a JSON file."""

    fig = go.Figure()
    col_idx = 0

    for file in files:
        with file.open() as fh:
            data = json.load(fh)

        col_idx = add_line(fig, data, "combined", f"CPB - {file.name}", col_idx)
        col_idx = add_line(fig, data, "base", f"CPB_B - {file.name}", col_idx)
        col_idx = add_line(fig, data, "enhancement", f"CPB_L - {file.name}", col_idx)

    fig.update_layout(showlegend=True, legend_title_text=None)
    fig.show()


if __name__ == "__main__":
    app()
