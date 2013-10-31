from ert_gui.ide.keywords.definitions import IntegerArgument, KeywordDefinition, ConfigurationLineDefinition, PathArgument, StringArgument, BoolArgument


class PlotKeywords(object):
    def __init__(self, ert_keywords):
        super(PlotKeywords, self).__init__()
        self.group = "Plot"

        ert_keywords.addKeyword(self.addImageViewer())
        ert_keywords.addKeyword(self.addImageType())
        ert_keywords.addKeyword(self.addPlotDriver())
        ert_keywords.addKeyword(self.addPlotErrorbar())
        ert_keywords.addKeyword(self.addPlotErrorbarMax())
        ert_keywords.addKeyword(self.addPlotWidth())
        ert_keywords.addKeyword(self.addPlotHeight())
        ert_keywords.addKeyword(self.addPlotRefcase())
        ert_keywords.addKeyword(self.addRefcaseList())
        ert_keywords.addKeyword(self.addPlotPath())
        ert_keywords.addKeyword(self.addRftConfig())
        ert_keywords.addKeyword(self.addRftPath())



    def addImageViewer(self):
        image_viewer = ConfigurationLineDefinition(keyword=KeywordDefinition("IMAGE_VIEWER"),
                                                    arguments=[PathArgument()],
                                                    documentation_link="plot/image_viewer",
                                                    required=False,
                                                    group=self.group)
        return image_viewer


    def addImageType(self):
        image_type = ConfigurationLineDefinition(keyword=KeywordDefinition("IMAGE_TYPE"),
                                                 arguments=[StringArgument(built_in=True)],
                                                 documentation_link="plot/image_type",
                                                 required=False,
                                                 group=self.group)
        return image_type


    def addPlotDriver(self):
        plot_driver = ConfigurationLineDefinition(keyword=KeywordDefinition("PLOT_DRIVER"),
                                                  arguments=[StringArgument(built_in=True)],
                                                  documentation_link="plot/plot_driver",
                                                  required=False,
                                                  group=self.group)
        return plot_driver


    def addPlotErrorbar(self):
        plot_errorbar = ConfigurationLineDefinition(keyword=KeywordDefinition("PLOT_ERRORBAR"),
                                                     arguments=[BoolArgument()],
                                                     documentation_link="plot/plot_errorbar",
                                                     required=False,
                                                     group=self.group)
        return plot_errorbar



    def addPlotErrorbarMax(self):
        plot_errorbar_max = ConfigurationLineDefinition(keyword=KeywordDefinition("PLOT_ERRORBAR_MAX"),
                                                        arguments=[IntegerArgument()],
                                                        documentation_link="plot/plot_errorbar_max",
                                                        required=False,
                                                        group=self.group)
        return plot_errorbar_max



    def addPlotWidth(self):
        plot_width = ConfigurationLineDefinition(keyword=KeywordDefinition("PLOT_WIDTH"),
                                                 arguments=[IntegerArgument()],
                                                 documentation_link="plot/plot_width",
                                                 required=False,
                                                 group=self.group)
        return plot_width


    def addPlotHeight(self):
        plot_height = ConfigurationLineDefinition(keyword=KeywordDefinition("PLOT_HEIGHT"),
                                                  arguments=[IntegerArgument()],
                                                  documentation_link="plot/plot_height",
                                                  required=False,
                                                  group=self.group)
        return plot_height



    def addPlotRefcase(self):
        plot_refcase = ConfigurationLineDefinition(keyword=KeywordDefinition("PLOT_REFCASE"),
                                                   arguments=[BoolArgument()],
                                                   documentation_link="plot/plot_refcase",
                                                   required=False,
                                                   group=self.group)
        return plot_refcase



    def addRefcaseList(self):
        refcase_list = ConfigurationLineDefinition(keyword=KeywordDefinition("REFCASE_LIST"),
                                                   arguments=[StringArgument(rest_of_line=True,allow_space=True)],
                                                   documentation_link="plot/refcase_list",
                                                   required=False,
                                                   group=self.group)
        return refcase_list


    def addPlotPath(self):
        plot_path = ConfigurationLineDefinition(keyword=KeywordDefinition("PLOT_PATH"),
                                                arguments=[PathArgument()],
                                                documentation_link="plot/plot_path",
                                                required=False,
                                                group=self.group)
        return plot_path


    def addRftConfig(self):
        rft_config = ConfigurationLineDefinition(keyword=KeywordDefinition("RFT_CONFIG"),
                                                 arguments=[PathArgument()],
                                                 documentation_link="plot/rft_config",
                                                 required=False,
                                                 group=self.group)
        return rft_config


    def addRftPath(self):
        rft_path = ConfigurationLineDefinition(keyword=KeywordDefinition("RFT_PATH"),
                                                 arguments=[PathArgument()],
                                                 documentation_link="plot/rft_path",
                                                 required=False,
                                                 group=self.group)
        return rft_path

