/// <reference types="node" />

/**
 * Represents an absolutely positioned rectangle on a page.
 *
 * Coordinates are in pts (1/72 of inch).
 */
export interface AbsRect {
    /** x coordinate (in pts) of the bottom left corner */
    x1: number
    /** x coordinate (in pts) of the upper right corner */
    x2: number
    /** y coordinate (in pts) of the bottom left corner */
    y1: number
    /** y coordinate (in pts) of the upper right corner */
    y2: number
}

/**
 * Represents a relatevly positioned rectangle on a page.
 *
 * Coordinates are relative to page width and height.
 * I.e. `0.5` means the middle of the page.
 */
export interface RelRect {
    /**
    * x coordinate of the bottom left corner relative
    * to the bottom left corner of the page.
    */
    x1: number
    /**
    * x coordinate of the upper right corner relative
    * to the bottom left corner of the page.
    */
    x2: number
    /**
    * y coordinate of the bottom left corner relative
    * to the bottom left corner of the page.
    */
    y1: number
    /**
    * y coordinate of the upper right corner relative
    * to the bottom left corner of the page.
    */
    y2: number
}

/**
 * Represents a word on a page.
 *
 * Stores the actual value of the word as well as its bouding box position.
 */
export interface Word extends RelRect {
    /** Actual value of this word */
    text: string,
}

/**
 * Represents a slice of a page.
 *
 * Coordinates are relative to the bottom left corner of the page.
 */
export interface Slice {
    /**
    * Relative `x` coordinate of the bottom left corner,
    * e.g `0.5` points to the horizontal middle of the page
    */
    x: number
    /**
    * Relative `y` coordinate of the bottom left corner,
    * e.g `0.5` points to the vertical middle of the page
    */
    y: number
    /**
    * Relative width of a slice,
    * e.g. `0.5` defines a half of a page width.
    */
    w: number
    /**
    * Relative height of a slice,
    * e.g. `0.5` defines a half of a page height.
    */
    h: number
}

/**
 * Represents a result of a `renderToFile` operation.
 */
export interface FileRenderResult {
    type: 'file',
    path: string,
}

/**
 * Represents a result of a `renderToBuffer` operation.
 */
export interface BufferRenderResult {
    type: 'buffer',
    format: 'png' | 'jpeg' | 'tiff',
    /** Raw image data. */
    data: Buffer,
}

export type RenderResult = FileRenderResult | BufferRenderResult

/**
 * Compression method for `tiff` format.
 *
 * Actual availability of different compression methods depends
 * on your environment so not all methods may be supported by
 * your setup.
 */
export type TiffCompression = 'none'
    | 'ccittrle'
    | 'ccittfax3'
    | 'ccittt4'
    | 'ccittfax4'
    | 'ccittt6'
    | 'lzw'
    | 'ojpeg'
    | 'jpeg'
    | 'next'
    | 'packbits'
    | 'ccittrlew'
    | 'deflate'
    | 'adeflate'
    | 'dcs'
    | 'jbig'
    | 'jp2000';


/**
 * Options for a render operation.
 */
export interface RenderOptions {
    /**
     * Number from 0 to 100. Works only for `jpeg` format.
     */
    quality?: number,
    /**
     * Compression method for `tiff` format.
     */
    compression?: TiffCompression,
    /**
     * Progressive `jpeg`.
     */
    progressive?: boolean,
    /**
     * Slice of a page to render instead of a full page.
     */
    slice?: Slice,
}

/**
 * PDF document.
 */
export class PopplerDocument {
    /** Is this document linearized? */
    isLinearized: boolean
    /** Is this document encrypted? */
    isEncrypted: boolean
    /** PDF version (e.g. `'PDF-1.6'`). */
    pdfVersion: string
    /** Nuber of pages in this document. */
    pageCount: number
    /** E.g. `1`. */
    PDFMajorVersion: number
    /** E.g. `6`. */
    PDFMinorVersion: number
    /** Path to a document file, if any. */
    fileName?: string

    /**
     * Constructor of a PDF document.
     * @param fileName string | Buffer path to the document or a memory byffer containing pdf data.
     * @param userPassword string? password required to open this document, if any.
     * @param ownerPassword string? password required to manipulate this document, if any.
     */
    constructor(fileName: string | Buffer, userPassword?: string, ownerPassword?: string);

    /**
     * This method will return a specified page if it exists in the document.
     * @param number number of desired page.
     */
    getPage(number: number): PopplerPage | null;
}

/**
 * Page of a PDF document.
 */
export class PopplerPage {
    /** Page number in a document. */
    num: number
    /** Widht of a page in pts (1/72 of inch). */
    height: number
    /** Height of a page in pts (1/72 of inch). */
    width: number
    /** Number of annotataion on this page. */
    numAnnots: number
    /** Crop box of this page. */
    crop_box: AbsRect
    /** Media box of this page. */
    media_box: AbsRect
    /** Art box of this page. */
    art_box: AbsRect
    /** Trim box of this page. */
    trim_box: AbsRect
    /** Bleed box of this page. */
    bleed_box: AbsRect
    /** Page rotation. */
    rotate: number
    /** Is this pages cropped? */
    isCropped: boolean

    /**
     * Renders page to a file syncronously.
     * @param path path to a file
     * @param format output file format
     * @param ppi resolution in pixels per inch
     * @param options render options
     */
    renderToFile(
        path: string,
        format: 'png' | 'jpeg' | 'tiff',
        ppi: number,
        options?: RenderOptions,
    ): FileRenderResult;

    /**
     * Renders page to a file asyncronously using old-fashioned CPS API.
     * @param path path to a file
     * @param format output file format
     * @param ppi resolution in pixels per inch
     * @param options render options
     * @param callback operation callback
     */
    renderToFile(
        path: string,
        format: 'png' | 'jpeg' | 'tiff',
        ppi: number,
        options?: RenderOptions,
        callback: (err: Error, result: FileRenderResult) => any,
    ): void;

    /**
     * Renders page to a file asyncronously. Returns `Promise`.
     * @param path path to a file
     * @param format output file format
     * @param ppi resolution in pixels per inch
     * @param options render options
     */
    renderToFileAsync(
        path: string,
        format: 'png' | 'jpeg' | 'tiff',
        ppi: number,
        options?: RenderOptions,
    ): Promise<FileRenderResult>;

    /**
     * Renders page to a buffer syncronously.
     * @param format output file format
     * @param ppi resolution in pixels per inch
     * @param options render options
     */
    renderToBuffer(
        format: 'png' | 'jpeg' | 'tiff',
        ppi: number,
        options?: RenderOptions,
    ): BufferRenderResult;

    /**
     * Renders page to a buffer asyncronously using old-fashioned CPS API.
     * @param format output file format
     * @param ppi resolution in pixels per inch
     * @param options render options
     * @param callback operation callback
     */
    renderToBuffer(
        format: 'png' | 'jpeg' | 'tiff',
        ppi: number,
        options?: RenderOptions,
        callback: (err: Error, result: BufferRenderResult) => any,
    ): void;

    /**
     * Renders page to a buffer asyncronously. Returns `Promise`.
     * @param format output file format
     * @param ppi resolution in pixels per inch
     * @param options render options
     */
    renderToBufferAsync(
        format: 'png' | 'jpeg' | 'tiff',
        ppi: number,
        options?: RenderOptions,
    ): Promise<BufferRenderResult>;

    /**
     * This method tries to find `text` on this page.
     * @param text text to search
     * @returns list of found rectangles
     */
    findText(text: string): RelRect[];

    /**
     * This method will return list of all words on this page.
     */
    getWordList(): Word[];

    /**
     * It's a way to "highlight" one or multiple rectangles on a page.
     * @param rectangles desired positions for annotations
     */
    addAnnot(rectangles: RelRect | RelRect[]): void;

    /**
     * Removes annotations created using `addAnnot(..)`.
     */
    deleteAnnots(): void;
}
