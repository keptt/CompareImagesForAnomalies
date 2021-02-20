#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <utility>
#include <tuple>
#include <exception>
#include <typeinfo>
#include <stdexcept>


namespace stbimg {
	#define STB_IMAGE_IMPLEMENTATION
	#include "stb_image.h"
	#define STB_IMAGE_WRITE_IMPLEMENTATION
	#include "stb_image_write.h"
}


#include "custom_exceptions.h"


int g_saved_imgs_counter = 0;
const int g_relation = 1;

const char *SAVED_IMG_EXTENSION			= "png";
const char *DEFAULT_RESULT_IMAGE_NAME	= "result";



std::string slice_basedir(const std::string &str, bool filename_exists=false) {
	size_t found = str.find_last_of("/\\");

	if (found == std::string::npos) {
		if (filename_exists) {		// if the only element in path is name of the file - it means basedir is ""
			return "";
		}
		return str;
	}

	return str.substr(0, found);
}


std::string slice_filename(const std::string &str) {
	size_t found = str.find_last_of("/\\");

	if (found == std::string::npos) {
		return str;
	}

	return str.substr(found + 1);
}


std::string remove_extension(const std::string &filename) {
	size_t lastdot = filename.find_last_of(".");

	if (lastdot == std::string::npos) {
		return filename;
	}

	return filename.substr(0, lastdot);
}


std::string get_basefilename(const std::string &filepath) {	// get filename without extension from path
	std::string filename = slice_filename(filepath);

	return remove_extension(filename);
}


std::string append_extension_if_not_exists(const std::string &path, const std::string &extension) {
	std::string filename = slice_filename(path);
	size_t lastdot = filename.find_last_of(".");

	if (lastdot == 0) {								// npos if not found and lastdot if unix hidden file like .env
		lastdot = std::string::npos;
	}

	return path.substr(0, lastdot) + "." + extension;
}


std::string form_grayscaled_img_path(const std::string &basefilename, const std::string &extension, const std::string &basepath) {
	return ( basepath.length() ? (basepath + "/" ) : "" ) + basefilename + "_gray." + extension;
}


std::string form_result_img_path(const std::string	&saved_img_extension
								, const std::string &result_img_save_path="\0"
								, const std::string &default_save_path="\0"
								, const std::string &default_filename_without_extension="\0"
								, int saved_imgs_counter=-1
) throw(cex::NoImageSavePath) {
	if (result_img_save_path == "\0" && default_save_path == "\0") {
		// return result_img_save_path; //! or throw
		throw new cex::NoImageSavePath();
	}

	if (result_img_save_path != "\0") {
		return append_extension_if_not_exists(result_img_save_path, saved_img_extension);
	}

	if (default_save_path == "\0" || default_filename_without_extension == "\0") {
		// return default_save_path; //! or throw
		throw new cex::NoImageSavePath();
	}

	return default_save_path + '/' + default_filename_without_extension + ((saved_imgs_counter != -1) ? ("_" + std::to_string(saved_imgs_counter)) : "") + saved_img_extension;
}


double form_map_key(unsigned char *p, int channels) {
	return (double)((int)*p * 1000 + (channels == 1 ? 0 : (int)*(p + 1)));
}


unsigned char* grayscale_img(unsigned char *img
							, const  int width
							, const  int height
							, const  int channels
							, int	 &gray_channels
							, size_t &gray_img_size
							, const bool ignore_4_channels=true
) throw(cex::ImageMemoryAllocationError) {
	size_t img_size = width * height * channels;
	gray_channels = ignore_4_channels ? 1 : ((channels == 4) ? 2 : 1);
	gray_img_size = width * height * gray_channels;

	unsigned char *gray_img = new unsigned char[gray_img_size];
	if (gray_img == NULL) {
		// std::cout << "Unable to allocate memory for the gray img.\n";
		// exit(1);
		throw new cex::ImageMemoryAllocationError("Unable to allocate memory for the gray img.\n");
	}


	if (ignore_4_channels) {
		for (unsigned char *p = img, *pg = gray_img; p != img + img_size; p += channels, pg += gray_channels) {
			*pg = (uint8_t)((*p + *(p + 1) + *(p + 2)) / 3.0);
		}
		return gray_img;
	}

	for (unsigned char *p = img, *pg = gray_img; p != img + img_size; p += channels, pg += gray_channels) {
		*pg = (uint8_t)((*p + *(p + 1) + *(p + 2)) / 3.0);
		if (channels == 4) {
			// std::cout << "transparency *(p + 3): " << *(p + 3) << "\n";
			*(pg + 1) = *(p + 3);
		}
	}

	return gray_img;
}



//void get_frequency_map(std::map<int, double>* shade_frequencies_map, unsigned char* gray_img, const int gray_channels, const size_t gray_img_size) {
//	if (gray_channels == 2) {
//		for (unsigned char *p = gray_img; p != gray_img + gray_img_size; p += gray_channels) {
//			(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
//		}
//	}
//	else if (gray_channels == 1) {
//		for (unsigned char *p = gray_img; p != gray_img + gray_img_size; p += gray_channels) {
//			(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
//		}
//	}
//
//	for (auto const &el : (*shade_frequencies_map)) {
//		(*shade_frequencies_map)[el.first] = (el.second / gray_img_size) * 100;
//	}
//
//	//std::cout << "=======================================================\n";
//	//for (auto const &el : (*shade_frequencies_map)) {
//	//	std::cout << el.first << " -> " << el.second << "\n";
//	//}
//	//std::cout << "=======================================================\n\n";
//}

void get_frequency_map(std::map<int, double>* shade_frequencies_map, unsigned char* gray_img, const size_t gray_img_size, const int number_of_dims=1, const int gray_channels=1) {
	if (gray_channels != 1 && gray_channels != 2) {
		throw;
	}
	if (number_of_dims < 1 && number_of_dims > 6) {
		throw;
	}
	for (unsigned char* p = gray_img; p != gray_img + gray_img_size; p += gray_channels * number_of_dims) {
		if (gray_channels == 1) {
			switch (number_of_dims) {
			case 1:
				(*shade_frequencies_map)[form_map_key(p, number_of_dims, gray_channels)] += 1;
				break;
			case 2:
				(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
				break;
			case 3:
				(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
				break;
			case 4:
				(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
				break;
			case 5:
				(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
				break;
			case 6:
				(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
				break;
			default:
				return;
			}
		} else if (gray_channels == 2) {
			switch (number_of_dims) {
			case 1:
				(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
				break;
			case 2:
				(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
				break;
			case 3:
				(*shade_frequencies_map)[form_map_key(p, gray_channels)] += 1;
				break;
			}
		}
	}

	for (auto const& el : (*shade_frequencies_map)) {
		(*shade_frequencies_map)[el.first] = (el.second / gray_img_size) * 100;
	}

}


unsigned char* load_img(const std::string& filename, int& width, int& height, int& channels) throw(cex::ImageLoadingError) {
	unsigned char *img = stbimg::stbi_load(filename.c_str(), &width, &height, &channels, 0);
	if (img == NULL) {
		// std::cout << "Error during loading the first img\n";
		// exit(1);
		throw new cex::ImageLoadingError("Error during loading of '" + filename + "'\n");
	}

	return img;
}


void get_comparison_frequency_map(std::map<int, double>* img_pathology_shade_frequencies_map, std::map<int, double>* img_norm_shade_frequencies_map) {
	for (const auto& el : (*img_pathology_shade_frequencies_map)) {
		int key = el.first;
		if ((*img_norm_shade_frequencies_map).count(key)) {
			double norm_shade_frequency = (*img_norm_shade_frequencies_map)[key] != 0 ? (*img_norm_shade_frequencies_map)[key] : 1.0;
			(*img_pathology_shade_frequencies_map)[key] = (*img_pathology_shade_frequencies_map)[key] / norm_shade_frequency;
		}
		else {
			(*img_pathology_shade_frequencies_map)[key] = 2.0; // means that shade of gray is suitable to be picked
		}
	}

	//std::cout << "===============================COMPARISON FREQUENCY===================================\n";
	//for (auto const& el : (*img_pathology_shade_frequencies_map)) {
	//	std::cout << el.first << " -> " << el.second << "\n";
	//}
	//std::cout << "=======================================================\n\n";
}


void highlight_img(unsigned char* img_pathology_gray
					, const size_t img_gray_size
					, const size_t img_gray_channels
					, std::map<int, double>& compared_shade_frequencies_map
					, int	relation_coef=g_relation
) {
	for (unsigned char* p = img_pathology_gray; p != img_pathology_gray + img_gray_size; p += img_gray_channels) {
		if (img_gray_channels == 2) {
			*p = compared_shade_frequencies_map[form_map_key(p, img_gray_channels)] > g_relation ? *p : 0; //TODO: handle index not found?
			*(p + 1) = compared_shade_frequencies_map[form_map_key(p, img_gray_channels)] > g_relation ? *(p + 1) : 1;
		}
		else if (img_gray_channels == 1) {
			*p = compared_shade_frequencies_map[form_map_key(p, img_gray_channels)] > 1 ? *p : 0;
		}
	}
}


void compare_imgs(const std::string &path_img_norm
				, const std::string &path_img_pathology
				, const std::string &result_img_save_path="\0"
				, const bool		save_grayscale_imgs=false
				, const std::string &grayscale_imgs_path="\0"
) throw() {
	int width_img_norm, height_img_norm, channels_img_norm
		, width_img_pathology, height_img_pathology, channels_img_pathology;

	unsigned char* img_norm, * img_pathology;
	img_norm		= load_img(path_img_norm, width_img_norm, height_img_norm, channels_img_norm); 						// TODO: create namespace for functions in the image processing library
	img_pathology	= load_img(path_img_pathology, width_img_pathology, height_img_pathology, channels_img_pathology); 	// TODO: create namespace for functions in the image processing library

	std::cout << "Loaded img Norm with a width of " << width_img_norm << "px, a height of " << height_img_norm << "px and " << channels_img_norm << " channels\n";
	std::cout << "Loaded img Pathology with a width of " << width_img_pathology << "px, a height of " << height_img_pathology << "px and " << channels_img_pathology << " channels\n";

	size_t	img_norm_size = width_img_norm * height_img_norm * channels_img_norm;
	int		img_norm_gray_channels			= NULL;
	int		img_pathology_gray_channels		= NULL;
	size_t	img_norm_gray_img_size			= NULL;
	size_t	img_pathology_gray_size			= NULL;
	unsigned char* img_norm_gray, * img_pathology_gray;
	try {
		img_norm_gray = grayscale_img(img_norm
			, width_img_norm
			, height_img_norm
			, channels_img_norm
			, img_norm_gray_channels
			, img_norm_gray_img_size
		);
		img_pathology_gray = grayscale_img(img_pathology
			, width_img_pathology
			, height_img_pathology
			, channels_img_pathology
			, img_pathology_gray_channels
			, img_pathology_gray_size
		);
	}
	catch (const cex::ImageMemoryAllocationError& exc) {
		throw new cex::ImageMemoryAllocationError("Error memory allocation for grayscaled image version of '" + path_img_norm + "' or '" + path_img_pathology + "'\n");
	}

	if (img_norm_gray_channels != img_pathology_gray_channels) {
		std::string error_message = "Gray channels in imgs" + path_img_norm + " and " + path_img_pathology + "are of different sizes (" + std::to_string(img_norm_gray_channels) + " and " + std::to_string(img_pathology_gray_channels) + ")\n";
		std::cout << error_message;
		// exit(1);
		throw cex::ChannelInconsistency(error_message);
	}

	std::cout << "First img_size: " << img_norm_gray_img_size << " gray_channels: " << img_norm_gray_channels << "\n";
	std::cout << "First img_size: " << img_pathology_gray_size << " gray_channels: " << img_norm_gray_channels << "\n";


	if (save_grayscale_imgs) {
		std::string img_norm_gray_path		= form_grayscaled_img_path(get_basefilename(path_img_norm), SAVED_IMG_EXTENSION, (grayscale_imgs_path == "\0") ? slice_basedir(path_img_norm, true) : grayscale_imgs_path);
		std::string img_pathology_gray_path = form_grayscaled_img_path(get_basefilename(path_img_pathology), SAVED_IMG_EXTENSION, (grayscale_imgs_path == "\0") ? slice_basedir(path_img_pathology, true) : grayscale_imgs_path);

		const char* a1 = img_norm_gray_path.c_str();
		const char* a2 = img_pathology_gray_path.c_str();

		std::cout << "a1: " << a1 << std::endl;
		std::cout << "a2: " << a2 << std::endl;

		stbimg::stbi_write_png(img_norm_gray_path.c_str()
								, width_img_norm
								, height_img_norm
								, img_norm_gray_channels
								, img_norm_gray
								, width_img_norm * img_norm_gray_channels
		);
		stbimg::stbi_write_png(img_pathology_gray_path.c_str()
								, width_img_pathology
								, height_img_pathology
								, img_norm_gray_channels
								, img_pathology_gray
								, width_img_pathology * img_norm_gray_channels
		);
		//stbi_write_jpg("img1_gray.jpg", width_img_norm, height_img_norm, img_norm_gray_channels, img_norm_gray, 100);
		//stbi_write_jpg("img2_gray.jpg", width_img_pathology, height_img_pathology, img_norm_gray_channels, img_pathology_gray, 100);
	}

	delete[]img_norm;
	delete[]img_pathology;

	std::map<int, double> img_norm_shade_frequencies_map		= {};
	std::map<int, double> img_pathology_shade_frequencies_map	= {};

	get_frequency_map(&img_norm_shade_frequencies_map
					, img_norm_gray
					, img_norm_gray_channels
					, img_norm_gray_img_size
	);
	get_frequency_map(&img_pathology_shade_frequencies_map
					, img_pathology_gray
					, img_pathology_gray_channels
					, img_pathology_gray_size
	);

	std::cout << "Norm img map size:" << img_norm_shade_frequencies_map.size() << '\n';
	std::cout << "Pathology img map size:" << img_pathology_shade_frequencies_map.size() << '\n';

	delete[]img_norm_gray;

	std::map<int, double>& compared_shade_frequencies_map = img_pathology_shade_frequencies_map;
	get_comparison_frequency_map(&compared_shade_frequencies_map, &img_norm_shade_frequencies_map);

	highlight_img(img_pathology_gray, img_pathology_gray_size, img_pathology_gray_channels, compared_shade_frequencies_map);

	//stbi_write_jpg("result.jpg", width_img_pathology, height_img_pathology, img_norm_gray_channels, img_pathology_gray, 100);

	std::string result_path_copy = form_result_img_path(SAVED_IMG_EXTENSION
														, result_img_save_path
														, slice_basedir(path_img_pathology)
														, DEFAULT_RESULT_IMAGE_NAME
														, g_saved_imgs_counter++
												);
	stbimg::stbi_write_png(result_path_copy.c_str()
							, width_img_pathology
							, height_img_pathology
							, img_norm_gray_channels
							, img_pathology_gray
							, width_img_pathology * img_norm_gray_channels
					);

	delete[]img_pathology_gray;
}


int main() {
	std::vector< std::tuple<const std::string, const std::string, const std::string> > img_pathes = {
		{"image1.png", "image2.png", "result"}
	};

	for (const auto imgs_info : img_pathes) {
		try {
			compare_imgs(std::get<0>(imgs_info), std::get<1>(imgs_info), std::get<2>(imgs_info), true);
		}
		catch (const std::exception &exc) {
			std::cout << "EXCEPTION\n";

			std::cout << exc.what() << std::endl;

			return 1;
		} catch (...) {
			std::exception_ptr p = std::current_exception();

			std::cout << "EXCEPTION\n";

			//std::clog << (p ? p.__cxa_exception_type()->name() : "null") << std::endl;

			std::cout << "Too bad, something unexpected appened :(\n";

			return 1;
		}
	}

	return 0;
}
